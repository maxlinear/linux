// SPDX-License-Identifier: GPL-2.0
/*****************************************************************************
 * Copyright (c) 2024, MaxLinear, Inc.
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.

*******************************************************************************/

#include <linux/soc/mxl/datapath_api.h>
#include "../datapath.h"
#include "../datapath_instance.h"
#include "datapath_ppv4.h"
#include "datapath_misc.h"
#include "datapath_tx.h"
#include "datapath_ppv4_session.h"
#include "datapath_spl_conn.h"

#include <linux/bitfield.h>
#include <linux/soc/mxl/pp_api.h>

/* disable optimization in debug mode: push */
DP_NO_OPTIMIZE_PUSH

#define DP_GSWIP_CRC_DISABLE 1
#define DP_GSWIP_CRC_ENABLE 0
#define DUMP_BUF_SIZE 100

//GSW_ENABLE need to adapt for IPPU/EPPU API
static struct pon_spl_cfg pon_cfg[] = {
#ifdef TOPAZ_CODE_ENABLE
	{
		.flag = DP_F_DEREGISTER,
		.ingress = 0,
		.egress = 1,
		.crc_check = DP_GSWIP_CRC_ENABLE,
		.fcs_gen = GSW_CRC_PAD_INS_EN,
		.flow_ctrl = GSW_FLOW_RXTX,
		.link_sts = GSW_PORT_LINK_AUTO,
		.tx_spl_tag = TX_SPTAG_REMOVE,
		.rx_time_stamp = RX_TIME_NOTS,
		.rx_spl_tag = RX_SPTAG_INSERT,
		.rx_fcs = RX_FCS_REMOVE,
		.pmac = PMAC_2,
		.qid = 2
	},
	{
		.flag = DP_F_FAST_ETH_WAN | DP_F_GPON,
		.ingress = 1,
		.egress = 1,
		.crc_check = DP_GSWIP_CRC_DISABLE,
		.fcs_gen = GSW_CRC_PAD_INS_DIS,
		.flow_ctrl = GSW_FLOW_OFF,
		.link_sts = GSW_PORT_LINK_UP,
		.tx_spl_tag = TX_SPTAG_REPLACE,
		.rx_time_stamp = RX_TIME_NO_INSERT,
		.rx_spl_tag = RX_SPTAG_NO_INSERT,
		.rx_fcs = 0,
		.pmac = PMAC_1,
		.qid = 28
	},
#endif
};

/* DPM allocate qos port for those device which directly dequeue packets
 * from QOS port, like PON device
 *  1) 0 ~ DP_MAX_CQM_DEQ-1: it is reserved to map it to CQM dequeue ring,
 *                                and no need to allocate
 *  2) DP_MAX_CQM_DEQ ~ DP_MAX_PPV4_PORT-1: for DPM to dynamcally allocate
 * [in] num: number of continuous ports to be allocated
 * return: base qos port id on success and -1 on fail
 */
int alloc_qos_port(int num)
{
//TODO: VBOLLA
	return 0;
}

/* to free dynamically allocated qos port:
 * [in] qos_port_base: valid between DP_MAX_CQM_DEQ ~ DP_MAX_PPV4_PORT-1
 * [in] num: number of continuous ports to be allocated
 * return: 0 on success and -1 on fail
 */
int free_qos_port(int qos_port_base, int num)
{
//TODO: VBOLLA
	return 0;
}

static inline
void set_tx_desc_pmac_bits(struct dp_port_info *dp_info, int templ,
			u32 flags, int val, bool is_mask)
{
	int i = 0;
	unsigned long bits = flags;
	struct adp_tx_desc_0 *dw0 = NULL;
	struct adp_tx_desc_1 *dw1 = NULL;
	struct adp_tx_desc_2 *dw2 = NULL;
	struct adp_tx_desc_3 *dw3 = NULL;

	if (is_mask) {
		dw0 = (struct adp_tx_desc_0 *)&dp_info->desc_dw_mask[templ][0];
		dw1 = (struct adp_tx_desc_1 *)&dp_info->desc_dw_mask[templ][1];
		dw2 = (struct adp_tx_desc_2 *)&dp_info->desc_dw_mask[templ][2];
		dw3 = (struct adp_tx_desc_3 *)&dp_info->desc_dw_mask[templ][3];
	} else {
		dw0 = (struct adp_tx_desc_0 *)&dp_info->desc_dw_templ[templ][0];
		dw1 = (struct adp_tx_desc_1 *)&dp_info->desc_dw_templ[templ][1];
		dw2 = (struct adp_tx_desc_2 *)&dp_info->desc_dw_templ[templ][2];
		dw3 = (struct adp_tx_desc_3 *)&dp_info->desc_dw_templ[templ][3];
	}

	for_each_set_bit(i, &bits, __bf_shf(DESC_TX_TEMPL_END)) {
		if (BIT(i) == DESC_TX_EGFLAG)
			dw2->egflag = val;
		if (BIT(i) == DESC_TX_CKSUM_GEN)
			dw1->csum_gen_en = val;
		if (BIT(i) == DESC_TX_CKSUM_VER)
			dw1->csum_ver_en = val;
		if (BIT(i) == DESC_TX_GPID)
			dw3->port = val;
		if (BIT(i) == DESC_TX_PTP)
			dw0->pon.ptp.ptp = val;
		if (BIT(i) == DESC_TX_DW0_ALL)
			dw0->all = val;
		if (BIT(i) == DESC_TX_DW1_ALL)
			dw1->all = val;
		if (BIT(i) == DESC_TX_DW2_ALL)
			dw2->all = val;
		if (BIT(i) == DESC_TX_DW3_ALL)
			dw3->all = val;
	}
}

static inline
void set_rx_desc_pmac_bits(struct dp_port_info *dp_info, int templ,
			u32 flags, int val, bool is_mask)
{
	int i = 0;
	unsigned long bits = flags;
	struct adp_rx_desc_0 *dw0 = NULL;
	struct adp_rx_desc_1 *dw1 = NULL;
	struct adp_rx_desc_2 *dw2 = NULL;
	struct adp_rx_desc_3 *dw3 = NULL;

	if (is_mask) {
		dw0 = (struct adp_rx_desc_0 *)&dp_info->desc_dw_mask[templ][0];
		dw1 = (struct adp_rx_desc_1 *)&dp_info->desc_dw_mask[templ][1];
		dw2 = (struct adp_rx_desc_2 *)&dp_info->desc_dw_mask[templ][2];
		dw3 = (struct adp_rx_desc_3 *)&dp_info->desc_dw_mask[templ][3];
	} else {
		pr_err("DPM: %s, Currently RX norm template not supported, only RX mask templ is supported\n",
				__func__);
		dw0 = (struct adp_rx_desc_0 *)&dp_info->desc_dw_templ[templ][0];
		dw1 = (struct adp_rx_desc_1 *)&dp_info->desc_dw_templ[templ][1];
		dw2 = (struct adp_rx_desc_2 *)&dp_info->desc_dw_templ[templ][2];
		dw3 = (struct adp_rx_desc_3 *)&dp_info->desc_dw_templ[templ][3];
	}

	for_each_set_bit(i, &bits, __bf_shf(DESC_RX_TEMPL_END)) {
		if (BIT(i) == DESC_RX_OWN)
			dw3->own = val;
		if (BIT(i) == DESC_RX_SOP)
			dw1->sop = val;
		if (BIT(i) == DESC_RX_EOP)
			dw1->eop = val;
		if (BIT(i) == DESC_RX_DIC)
			dw1->dic = val;
		if (BIT(i) == DESC_RX_DW0_ALL)
			dw0->all = val;
		if (BIT(i) == DESC_RX_DW1_ALL)
			dw1->all = val;
		if (BIT(i) == DESC_RX_DW2_ALL)
			dw2->all = val;
		if (BIT(i) == DESC_RX_DW3_ALL)
			dw3->all = val;
	}
}

static inline
void save_adp_templ(struct dp_port_info *dp_info, int templ, u32 flags,
		    int val)
{
	if (templ != TEMPL_RX_NORMAL)
		set_tx_desc_pmac_bits(dp_info, templ, flags, val, false);
	else {
		pr_err("DPM: %s, Currently RX norm template not supported, only RX mask templ is supported\n",
				__func__);
	}
}

static inline
void save_adp_mask_templ(struct dp_port_info *dp_info, int templ, u32 flags,
		    int val)
{
	templ == TEMPL_RX_NORMAL ?
		set_rx_desc_pmac_bits(dp_info, templ, flags, val, true):
		set_tx_desc_pmac_bits(dp_info, templ, flags, val, true);
}

static void init_adp_template_subif(int portid, u32 vap)
{
	struct dp_subif_info *sif;
	struct dp_port_info *port_info = get_dp_port_info(0, portid);
	//unsigned long bits = port_info->alloc_flags;
	struct adp_tx_desc_3 *dw3 = NULL;
	int i;

	sif = get_dp_port_subif(port_info, vap);
	for (i = 0; i < TEMPL_MAX; i++) {
		if (i == TEMPL_RX_NORMAL)
			continue;
		dw3 = (struct adp_tx_desc_3 *)&sif->dw3[i];
		/*copy port level templ to subif level dw3 templ value*/
		dw3->all = port_info->desc_dw_templ[i][3];
		/*update subif gpid in the subif dw3 template*/
		if ((i == TEMPL_TX_CKSUM) ||  (i == TEMPL_TX_PTP_CKSUM))
			dw3->port = port_info->gpid_spl;
		else
			dw3->port = sif->gpid;
	}
}

static void init_adp_template_port(int portid, u32 flags)
{
	int i = 0;
	struct dp_port_info *dp_info = get_dp_port_info(0, portid);
	unsigned long bits = flags;
	u32 desc_flags;

	dp_memset(dp_info->desc_dw_templ, 0, sizeof(dp_info->desc_dw_templ));
	dp_memset(dp_info->desc_dw_mask, 0, sizeof(dp_info->desc_dw_mask));

	for (i = 0; i < TEMPL_MAX; i++) {
		/*Set all the template mask (TX and RX) set all bits to 1's*/
		if (i == TEMPL_RX_NORMAL)
			desc_flags = DESC_RX_DW0_ALL | DESC_RX_DW1_ALL | DESC_RX_DW2_ALL | DESC_RX_DW3_ALL;
		else
			desc_flags = DESC_TX_DW0_ALL | DESC_TX_DW1_ALL | DESC_TX_DW2_ALL | DESC_TX_DW3_ALL;
		save_adp_mask_templ(dp_info, i, desc_flags, 0xFFFFFFFF);
	}

	for_each_set_bit(i, &bits, __bf_shf(DP_F_DEV_END)) {
		switch (i) {
		case __bf_shf(DP_F_FAST_ETH_LAN):
		case __bf_shf(DP_F_FAST_ETH_WAN):
		case __bf_shf(DP_F_GPON):
		case __bf_shf(DP_F_GINT):
			/*TX NORM TEMPLATE*/
			save_adp_mask_templ(dp_info, TEMPL_TX_NORMAL,
					DESC_TX_EGFLAG | DESC_TX_CKSUM_GEN | DESC_TX_GPID, 0);
			save_adp_templ(dp_info, TEMPL_TX_NORMAL, DESC_TX_EGFLAG, 1);
			save_adp_templ(dp_info, TEMPL_TX_NORMAL, DESC_TX_CKSUM_GEN, 0);

			/*TX PTP TEMPLATE*/
			save_adp_mask_templ(dp_info, TEMPL_TX_PTP,
					DESC_TX_EGFLAG | DESC_TX_CKSUM_GEN |
					DESC_TX_PTP | DESC_TX_GPID, 0);
			save_adp_templ(dp_info, TEMPL_TX_PTP, DESC_TX_EGFLAG, 1);
			save_adp_templ(dp_info, TEMPL_TX_PTP, DESC_TX_PTP, 1);
			save_adp_templ(dp_info, TEMPL_TX_PTP, DESC_TX_CKSUM_GEN, 0);

			/*TX IPPU TEMPLATE*/
			save_adp_mask_templ(dp_info, TEMPL_TX_CKSUM,
					DESC_TX_EGFLAG | DESC_TX_CKSUM_GEN | DESC_TX_GPID, 0);
			save_adp_templ(dp_info, TEMPL_TX_CKSUM,
				       (DESC_TX_EGFLAG | DESC_TX_CKSUM_GEN), 1);

			/*TX IPPU PTP TEMPLATE*/
			save_adp_mask_templ(dp_info, TEMPL_TX_PTP_CKSUM,
					DESC_TX_EGFLAG | DESC_TX_CKSUM_GEN |
					DESC_TX_PTP | DESC_TX_GPID, 0);
			save_adp_templ(dp_info, TEMPL_TX_PTP_CKSUM,
				       (DESC_TX_EGFLAG | DESC_TX_CKSUM_GEN |
					DESC_TX_PTP), 1);

			/*RX NORM TEMPLATE(valid only mask) TODO: VBOLLA*/
			save_adp_mask_templ(dp_info, TEMPL_RX_NORMAL,
					DESC_RX_EOP | DESC_RX_SOP | DESC_RX_OWN, 0);

			break;
		case __bf_shf(DP_F_ACA):
			save_adp_mask_templ(dp_info, TEMPL_TX_CKSUM,
					(DESC_TX_EGFLAG | DESC_TX_CKSUM_GEN | DESC_TX_GPID), 0);
			save_adp_templ(dp_info, TEMPL_TX_CKSUM,
					(DESC_TX_EGFLAG | DESC_TX_CKSUM_GEN), 1);
			/*RX NORM TEMPLATE(valid only mask) TODO: VBOLLA*/
			save_adp_mask_templ(dp_info, TEMPL_RX_NORMAL,
					DESC_RX_EOP | DESC_RX_SOP | DESC_RX_OWN | DESC_RX_DIC, 0);
			break;
		default:
			break;
		}
	}
}

static void init_adp_template(int portid, u32 flags, bool subif_flag)
{
	subif_flag ?
		init_adp_template_subif(portid, flags) :
		init_adp_template_port(portid, flags);
}

static
void print_adp_tx_desc(struct adp_tx_desc_0 *desc_0,
		    struct adp_tx_desc_1 *desc_1,
		    struct adp_tx_desc_2 *desc_2,
		    struct adp_tx_desc_3 *desc_3)
{
	DP_DUMP("ADP TX Descriptor:D0=0x%08x D1=0x%08x D2=0x%08x D3=0x%08x\n",
		*(u32 *)desc_0, *(u32 *)desc_1,
		*(u32 *)desc_2, *(u32 *)desc_3);
	DP_DUMP("DW0: 0x%08x\n", desc_0->all);
	DP_DUMP("DW1: %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d\n",
		"class", desc_1->class,
		"csum_ver_en", desc_1->csum_ver_en,
		"csum_gen_en", desc_1->csum_gen_en,
		"dic", desc_1->dic,
		"eop", desc_1->eop,
		"sop", desc_1->sop,
		"buffer_off", desc_1->buffer_offset,
		"src_pool", desc_1->src_pool);

	DP_DUMP("DW2: %s=%d %s=%d %s=%d %s=%d\n",
		"buffer_ptr", desc_2->buffer_ptr,
		"c", desc_2->c,
		"egflag", desc_2->egflag,
		"ts", desc_2->ts);

	DP_DUMP("DW3: %s=%d %s=%d %s=%d %s=%d %s=%d\n",
		"gpid", desc_3->port,
		"pool_policy", desc_3->pool_policy,
		"packet len", desc_3->pkt_len,
		"sp", desc_3->sp,
		"own", desc_3->own);
}

static
void print_adp_rx_desc(struct adp_rx_desc_0 *desc_0,
		    struct adp_rx_desc_1 *desc_1,
		    struct adp_rx_desc_2 *desc_2,
		    struct adp_rx_desc_3 *desc_3)
{
	DP_DUMP("ADP RX Descriptor:D0=0x%08x D1=0x%08x D2=0x%08x D3=0x%08x\n",
		*(u32 *)desc_0, *(u32 *)desc_1,
		*(u32 *)desc_2, *(u32 *)desc_3);
	DP_DUMP("DW0: 0x%08x\n", desc_0->all);
	DP_DUMP("DW1: %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d %s=%d\n",
		"class", desc_1->class,
		"csum_ver_en", desc_1->csum_ver_en,
		"csum_gen_en", desc_1->csum_gen_en,
		"dic", desc_1->dic,
		"eop", desc_1->eop,
		"sop", desc_1->sop,
		"buffer_off", desc_1->buffer_offset,
		"l4s_inflight", desc_1->l4s_eq_inflight,
		"src_pool", desc_1->src_pool);

	DP_DUMP("DW2: %s=%d %s=%d %s=%d %s=%d\n",
		"buffer_ptr", desc_2->buffer_ptr,
		"l4sv", desc_2->l4sv,
		"iq_egflag", desc_2->iq_egflag,
		"ts", desc_2->ts);

	DP_DUMP("DW3: %s=%d %s=%d %s=%d %s=%d %s=%d\n",
		"gpid", desc_3->port,
		"pool_policy", desc_3->pool_policy,
		"packet len", desc_3->pkt_len,
		"sp", desc_3->sp,
		"own", desc_3->own);
}

#define QSV_LPID_BIT_POS 	9
#define ADP_EGFLAG_POS 		((2*32) + 30)
#define ADP_CLASS_POS 		((1*32) +  0)
#define ADP_DEVQOS_POS 		((0*32) + 18)
#define ADP_CHMGEN_POS 		((1*32) +  6)
#define ADP_SOPEOP_POS 		((1*32) +  8)
#define ADP_SUBIF_POS 		((0*32) +  0)

char *dp_qsv_field_type(u8 adp_off, int size)
{
	if (!size)
		return "UNUSED";

	switch (adp_off) {
		case ADP_EGFLAG_POS:
			return "EGFLAG";
		case ADP_CLASS_POS:
			return "CLASS ";
		case ADP_DEVQOS_POS:
			return "DEVQOS";
		case ADP_CHMGEN_POS:
			return "CHMGEN";
		case ADP_SOPEOP_POS:
			return "SOPEOP";
		case ADP_SUBIF_POS:
			return "SUBIF ";
		default:
			return "UNKNWN";
	}
	return "UNKNWN";
}

static
void print_lookup_cfg(struct adp_tx_desc_0 *desc_0,
		      struct adp_tx_desc_1 *desc_1,
		      struct adp_tx_desc_2 *desc_2,
		      struct adp_tx_desc_3 *desc_3)
{
	int inst = 0;
	int lookup_idx, dp_port, i;
	struct hal_priv *priv = HAL(inst);
	struct cqm_lookup cqm_lookup;
	struct dp_port_info *port_info;
	int f_size, f_qsv_off, f_adp_off;
	uint16_t field_val;
	int desc_number, desc_off, calc_qsv_bits = 0;

	dp_port = priv->gp_dp_map[desc_3->port].dpid; /* get lpid from gpid*/
	port_info = get_dp_port_info(inst, dp_port);

	lookup_idx = (dp_port << QSV_LPID_BIT_POS);
	for (i = 0 ; i < CQM_LOOKUP_SEL_NUM; i++) {
		f_size = port_info->lookup_sel.sel[i].size;
		calc_qsv_bits += f_size;
		if (f_size) {
			f_qsv_off = port_info->lookup_sel.sel[i].dst_offset;
			f_adp_off = port_info->lookup_sel.sel[i].hd_offset;
			desc_number = f_adp_off/(sizeof(*desc_0) * 8);
			desc_off = f_adp_off % (sizeof(*desc_0) * 8);
			switch (desc_number) {
				case 0:
						    /*desc u32 val >> desc_off   bit mask of qsv size*/
					field_val = ((desc_0->all >> desc_off) & ((1 << f_size) - 1));
					break;
				case 1:
					field_val = ((desc_1->all >> desc_off) & ((1 << f_size) - 1));
					break;
				case 2:
					field_val = ((desc_2->all >> desc_off) & ((1 << f_size) - 1));
					break;
				case 3:
					field_val = ((desc_3->all >> desc_off) & ((1 << f_size) - 1));
					break;
				default:
					pr_err("DPM: %s, qsv field idx: %d, qsv field_size: %d, wrong number: %d for desc\n",
							__func__, i, f_size, desc_number);
					return;
			}
			//printk("DPM--------------> i: %d, f_size: %d, f_qsv_off: %d, f_adp_off: %d, desc: %d, desc_off: %d, field_val: 0x%hx\n",
			//		i, f_size, f_qsv_off, f_adp_off, desc_number, desc_off, field_val);
			lookup_idx |= (field_val << f_qsv_off);
		}
	}
	/*unselected or unused bits are treated as zeros, while forming QSV val*/
	cqm_lookup.index = lookup_idx;
	DP_DUMP("  lookup index formed using: [ LPID:4 | %s:%d | %s:%d | %s:%d | %s:%d ]\n",
			dp_qsv_field_type(port_info->lookup_sel.sel[0].hd_offset,
				port_info->lookup_sel.sel[0].size),
			port_info->lookup_sel.sel[0].size,
			dp_qsv_field_type(port_info->lookup_sel.sel[1].hd_offset,
				port_info->lookup_sel.sel[1].size),
			port_info->lookup_sel.sel[1].size,
			dp_qsv_field_type(port_info->lookup_sel.sel[2].hd_offset,
				port_info->lookup_sel.sel[2].size),
			port_info->lookup_sel.sel[2].size,
			dp_qsv_field_type(port_info->lookup_sel.sel[3].hd_offset,
				port_info->lookup_sel.sel[3].size),
			port_info->lookup_sel.sel[3].size ?
				port_info->lookup_sel.sel[3].size :
				QSV_LPID_BIT_POS - calc_qsv_bits);
	DP_DUMP("  lookup index=0x%x qid=%d for gpid=%u, lpid: %d\n", lookup_idx,
		dp_get_lookup_qid_via_index(inst, &cqm_lookup),
		desc_3->port, dp_port);
}

/* Explicitely configure reserved GPID<->LPID mapping in CQM */
static int dp_cqm_set_reserved_gpid_map(int inst)
{
	struct cqm_gpid_lpid *cqm_gpid;
	int i;

	cqm_gpid = dp_kzalloc(sizeof(*cqm_gpid), GFP_ATOMIC);
	if (!cqm_gpid)
		return DP_FAILURE;
	cqm_gpid->cqm_inst = inst;
	for (i = DP_RES_GPID_LPID_START; i <= DP_RES_GPID_LPID_END; i++) {
		cqm_gpid->gpid = i;
		cqm_gpid->lpid = i;
		if (dp_cqm_gpid_lpid_map(inst, cqm_gpid)) {
			kfree(cqm_gpid);
			return DP_FAILURE;
		}
	}

	kfree(cqm_gpid);
	return 0;
}

static void dp_init_reserved_gpid_map(int inst)
{
	struct hal_priv *priv = HAL(inst);
	int i;

	for (i = DP_RES_GPID_LPID_START; i <= DP_RES_GPID_LPID_END; i++)
		priv->gp_dp_map[i].dpid = i;
}

static int dp_set_io_port(int inst, int dpid, int vap, int type)
{
	return __dp_update_hostif(inst, dpid, vap, type);
}

int dp_get_lookup_qid_via_index(int inst, struct cqm_lookup *info)
{
	int qid;

	if (!info)
		return 0;
	qid = dp_cqm_get_lookup_qid_via_index(inst, info);
	DP_DEBUG(DP_DBG_FLAG_LOOKUP, "get index=0x%x qid=%d\n",
			info->index, qid);
	return qid;
}

static enum pp_min_tx_pkt_len get_min_pkt_len_cfg(u32 len)
{
	static u32 min_pkt_len[PP_NUM_TX_MIN_PKT_LENS] = {
		PP_MIN_TX_PKT_LEN_VAL_NONE,
		PP_MIN_TX_PKT_LEN_VAL_60B,
		PP_MIN_TX_PKT_LEN_VAL_64B,
		PP_MIN_TX_PKT_LEN_VAL_128B,
	};
	int i;

	for (i = 0; i < PP_NUM_TX_MIN_PKT_LENS; i++) {
		if (len <= min_pkt_len[i])
			break;
	}
	return i;
}

static int __maybe_unused subif_platform_change_mtu(int inst, int portid, int subif_ix,
				     u32 mtu)
{
	return dp_subif_pp_change_mtu(inst, portid, subif_ix, mtu);
}

void dump_rx_adp_desc(struct adp_rx_desc_0 *desc_0,
		      struct adp_rx_desc_1 *desc_1,
		      struct adp_rx_desc_2 *desc_2,
		      struct adp_rx_desc_3 *desc_3)
{
	if (!desc_0 || !desc_1 || !desc_2 || !desc_3) {
		pr_err("DPM: %s: rx desc_0/1/2/3 NULL\n", __func__);
		return;
	}

	print_adp_rx_desc(desc_0, desc_1, desc_2, desc_3);
}

void dump_tx_adp_desc(struct adp_tx_desc_0 *desc_0,
		      struct adp_tx_desc_1 *desc_1,
		      struct adp_tx_desc_2 *desc_2,
		      struct adp_tx_desc_3 *desc_3)
{
	if (!desc_0 || !desc_1 || !desc_2 || !desc_3) {
		pr_err("DPM: %s: tx desc_0/1/2/3 NULL\n", __func__);
		return;
	}

	print_adp_tx_desc(desc_0, desc_1, desc_2, desc_3);
	print_lookup_cfg(desc_0, desc_1, desc_2, desc_3);
}

int alloc_q_to_port(struct ppv4_q_sch_port *info, u32 flag)
{
	struct dp_node_link link = {0};
	struct hal_priv *priv = HAL(info->inst);
	struct cqm_deq_ring_info *deq;

	link.cqm_deq_ring.cqm_deq_ring = info->cqe_deq;
	link.dp_port = info->dp_port;
	link.inst = info->inst;
	link.node_id.q_id = DP_NODE_AUTO_ID;
	link.node_type = DP_NODE_QUEUE;
	link.p_node_id.cqm_deq_ring = info->cqe_deq;
	link.p_node_type = DP_NODE_RING;
	link.arbi = ARBITRATION_PARENT;
	link.prio_wfq = 0;

	if (dp_node_link_add(&link, 0)) {
		pr_err("DPM: %s: dp_node_link_add fail: deq_ring=%d\n", __func__,
		       info->cqe_deq);
		return DP_FAILURE;
	}

	info->qid = link.node_id.q_id;
	info->q_node = priv->qos_queue_stat[info->qid].node_id;
	info->port_node = priv->deq_ring_stat[info->cqe_deq].node_id;

	deq = get_dp_deqring_info(info->inst, link.p_node_id.cqm_deq_ring);
	deq->qid = link.node_id.q_id;
	deq->q_node = info->q_node;

	DP_DEBUG(DP_DBG_FLAG_QOS,
		 "%s: qid=%d p_node_id=%d for cqm port ring=%d\n",
		 __func__, link.node_id.q_id, link.p_node_id.cqm_deq_ring,
		 info->cqe_deq);
	return DP_SUCCESS;
}

#define REINSERT BIT(1)

static
int alloc_cpu_q(int inst, struct cqm_cpu_port_data *cpu_data,
		struct ppv4_q_sch_port *q_port, int cpu_idx,
		int port_per_cpu, u32 flag)
{
	struct cqm_deq_ring_info *c_info;
	struct dp_cap *cap;
	struct dp_ring_pktdeq *deq = NULL;
	struct q_info *q_info;

	if (cpu_idx >= CQM_MAX_CPU)
		return DP_FAILURE;

	cap = &get_dp_prop_info(inst)->cap;

	q_port->inst = inst;
	q_port->dp_port = CPU_PORT;

	deq = &cpu_data->cpu_deq_info[cpu_idx][port_per_cpu];
	/* All CPU ports enabled have valid CQM Deq port otherwise -1 */
	if (deq->attr.ring.index == -1)
		return 1;
	q_port->cqe_deq = deq->attr.ring.index;
	q_port->tx_pkt_credit = deq->pkt_credit;
	q_port->tx_ring_addr = deq->txpush_addr_qos;
	q_port->tx_ring_addr_push = deq->attr.paddr;
	q_port->tx_ring_size = deq->attr.size;

	c_info = get_dp_deqring_info(inst, q_port->cqe_deq);

	/* Store Ring Info */
	c_info->deq = *deq;
	c_info->dp_port[CPU_PORT] = 1;
	c_info->ref_cnt++;

	/* store its vap*/
	q_port->vap = cpu_idx;

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
	if (dp_dbg_flag & DP_DBG_FLAG_QOS) {
		DP_DUMP("%s, cpu(%d)\n", __func__, cpu_idx);
		dp_dump_pktdeq_ring(0, deq, 1, false);
	}
#endif

	if (alloc_q_to_port(q_port, 0)) {
		pr_err("DPM: %s: alloc_q_to_port fail for dp_port=%d\n",
		       __func__, q_port->dp_port);
		return DP_FAILURE;
	}

	q_info = get_dp_q_info(inst, q_port->qid);
	q_info->flag = 1;
	q_info->need_free = 1;
	q_info->ref_cnt = 1;
	q_info->q_node_id = q_port->q_node;
	q_info->cqm_dequeue_ring = q_port->cqe_deq;
	c_info->qid = q_port->qid;

	return DP_SUCCESS;
}

static
void save_cpu_subif_info(int inst, struct cqm_cpu_port_data *cpu_data,
		     struct ppv4_q_sch_port *q_port,
		     struct dp_port_info *cpu_port,
		     u32 cpu_idx, u32 port_per_cpu, int vap,
		     struct dp_subif_info *subif_info)
{
	#define ring_idex(x, y, z) x->cpu_deq_info[y][z].attr.ring.index

	if (!subif_info->flags) {
		subif_info->flags = PORT_DEV_REGISTERED;
		subif_info->subif_num = 1;
		subif_info->subif = SET_VAP(vap, cpu_port->vap_offset,
				    cpu_port->vap_mask);
	}
	subif_info->num_qid++;
	subif_info->def_qid_list[port_per_cpu] = q_port->qid;
	subif_info->q_node[port_per_cpu] = q_port->q_node;
	subif_info->qos_deq_port[port_per_cpu] = q_port->port_node;
	subif_info->cqm_deq_ring[port_per_cpu] = q_port->cqe_deq;
	subif_info->num_deq_ring++;
	subif_info->deq_ring_idx = ring_idex(cpu_data, cpu_idx, port_per_cpu);

	if (!port_per_cpu) {
		/*only first time*/
		subif_info->tx_policy_base =
			cpu_data->cpu_deq_info[cpu_idx][port_per_cpu].policy_base;
		subif_info->tx_policy_num =
			cpu_data->cpu_deq_info[cpu_idx][port_per_cpu].num_policy;
		subif_info->tx_pkt_credit =
			cpu_data->cpu_deq_info[cpu_idx][port_per_cpu].pkt_credit;

		subif_info->spl_conn_type = DP_NON_SPL;
	}
	cpu_port->alloc_flags = DP_F_CPU;
	subif_info->port_info = cpu_port;

	subif_info->flags = 1;
}

static int dev_platform_set(int inst, u8 ep, struct dp_dev_data *data,
			    uint32_t flags)
{
	struct gsw_itf *itf;
	struct hal_priv *priv = (struct hal_priv *)dp_port_prop[inst].priv_hal;
	struct cqm_lookup_sel *lookup;
	struct dp_port_info *p = get_dp_port_info(inst, ep);

	if (!priv) {
		pr_err("DPM: %s: priv is NULL\n", __func__);
		return DP_FAILURE;
	}
	if (flags & DP_F_DEREGISTER)
		dp_free_deq_ring(inst, ep, data, flags);

	itf = dp_gsw_assign_ctp(inst, ep, flags, data);
	get_dp_port_info(inst, ep)->itf_info = itf;

	if (data) { /* For CPU_PORT data pointer is NULL*/
		lookup = dp_kzalloc(sizeof(*lookup), GFP_ATOMIC);
		if (!lookup)
			return DP_FAILURE;
		lookup->dpid = ep;
		/*User NOT provide lookup sel, so use default lookup sel*/
		dp_memcpy(lookup->sel, dp_get_dfl_cqm_lu_mode(p->cqe_lu_mode),
				sizeof(lookup->sel));
		/*send back lookup sel info to caller*/
		dp_memcpy(data->sel, lookup->sel, sizeof(lookup->sel));

		/*Set mode*/
		if (dp_cqm_qsv_set(dp_port_prop[inst].cqm_inst, lookup,
					flags)) {
			pr_err("DPM: %s: cqm_qsv_set failed\n", __func__);
			kfree(lookup);
			return DP_FAILURE;
		}
		/*Save configured lookup info in the dp_port_info table*/
		p->lookup_sel = *lookup;

		kfree(lookup);
	}
	dp_node_reserve(inst, ep, data, flags);

	if (gpid_port_assign(inst, ep, data, flags)) {
		pr_err("DPM: gpid_port_assign failed\n");
		return DP_FAILURE;
	}

	/*return gpid base to caller*/
	if (data)
		data->gpid_base = p->gpid[0];

	return DP_SUCCESS;
}

static int platform_map_to_drop_q(int inst)
{
	struct hal_priv *priv = (struct hal_priv *)dp_port_prop[inst].priv_hal;
	struct ppv4_queue *q;

	q = dp_kzalloc(sizeof(*q), GFP_ATOMIC);
	if (!q)
		return DP_FAILURE;
	/* Allocate a drop queue */
	if (priv->ppv4_drop_q < 0) {
		q->parent = 0;
		q->inst = inst;
		if (dp_pp_alloc_queue(q)) {
			kfree(q);
			return DP_FAILURE;
		}

		priv->ppv4_drop_q = q->qid;
	}
	dflt_q_drop[inst] = priv->ppv4_drop_q;
	_dp_reset_q_lookup_tbl(inst);

	kfree(q);
	return DP_SUCCESS;
}

static int cpu_mode_table_cfg(int inst)
{
	struct dp_port_info *cpu_port;
	struct cqm_lookup_sel *lookup;
	int cqm_inst = dp_port_prop[inst].cqm_inst;

	cpu_port = get_dp_port_info(inst, CPU_PORT);
	lookup = dp_kzalloc(sizeof(*lookup), GFP_ATOMIC);
	if (!lookup)
		return DP_FAILURE;
	lookup->dpid = CPU_PORT;
	dp_memcpy(lookup->sel,
		dp_get_dfl_cqm_lu_mode(cpu_port->cqe_lu_mode), sizeof(lookup->sel));
	if (dp_cqm_qsv_set(cqm_inst, lookup,
		    0)) {
		kfree(lookup);
		pr_err("DPM: %s: cqm_qsv_set failed\n", __func__);
		return DP_FAILURE;
	}

	kfree(lookup);
	return DP_SUCCESS;
}

static int dp_cpu_netdev(int inst, int cpu_id, int vap, int flag)
{
	char net_name[20];
	dp_subif_t *subif_sync __free(kfree) =
		dp_kmalloc(sizeof(*subif_sync), GFP_KERNEL);
	struct dp_port_info *cpu_port = get_dp_port_info(inst, CPU_PORT);
	struct dp_subif_info *sif;
	struct cqm_deq_ring_info *deq;
	int i;

	if (!subif_sync)
		return -1;

	sprintf(net_name, "cpu%d", cpu_id);
	sif = get_dp_port_subif(cpu_port, vap);
	sif->netif = dp_create_netdev(net_name);
	if (!sif->netif) {
		pr_err("dpm: dp_create_netdev failed\n");
		return -1;
	}

	subif_sync->inst = inst;

	/* copy common port flag */
	dp_memcpy(&subif_sync->port_cmn_start,
		  &cpu_port->port_cmn_start,
		  &subif_sync->port_cmn_end - &subif_sync->port_cmn_start);

	/* copy common subif info */
	dp_memcpy(&subif_sync->subif_cmn_start,
		  &sif->subif_cmn_start,
		  &subif_sync->subif_cmn_end - &subif_sync->subif_cmn_start);
	
	/* copy aggregated subif informations */
	/* set subifs */
	subif_sync->subif_num = 1;
	subif_sync->subif_list[0] = sif->subif;
	
	for (i = 0; i < sif->num_deq_ring; i++) {
		deq = get_dp_deqring_info(inst, sif->cqm_deq_ring[i]);
		subif_sync->deq[i] = deq->deq;
	}
	subif_sync->num_deq = sif->num_deq_ring;

	for (i = 0; i < sif->num_tx_ippu; i++) {
		deq = get_dp_deqring_info(inst, sif->tx_ippu_ring[i]);
		subif_sync->tx_ippu[i] = deq->deq;
	}
	subif_sync->num_tx_ippu = sif->num_tx_ippu;

	for (i = 0; i < sif->num_rx_ippu; i++) {
		deq = get_dp_deqring_info(inst, sif->rx_ippu_ring[i]);
		subif_sync->rx_ippu[i] = deq->deq;
	}
	subif_sync->num_rx_ippu = sif->num_rx_ippu;

	subif_sync->subif_groupid = vap;

	if (dp_update_subif(sif->netif, NULL, subif_sync, sif->netif->name, 0, NULL)) {
		pr_err("DPM: %s: dp_update_subif failed: %s\n", __func__,
		       sif->netif->name);
		return -1;
	}

	return 0;
}

static int dp_platform_queue_set(int inst, u32 flag)
{
	struct {
		struct cqm_cpu_port_data cpu_data;
		struct ppv4_q_sch_port q_port;
		struct cqm_dp_en_data en_data;
	} *p;
	struct hal_priv *priv = (struct hal_priv *)dp_port_prop[inst].priv_hal;
	struct dp_subif_info *subif_info;
	int cqm_inst = dp_port_prop[inst].cqm_inst;
	struct dp_dflt_hostif hostif = {0};
	struct dp_port_info *cpu_port;
	struct cqm_deq_ring_info *deq;
	int cpu_id, cpu_port_num, i, j;
	int ret = DP_FAILURE;
	struct dp_cap *cap;
	u8 f_cpu_q = 0;
	u32 flags = 0;
	int vap = 0;
	int deq_idx;
	int res;
	bool need_netdev;

	cpu_port = get_dp_port_info(inst, CPU_PORT);
	if (flag & DP_PLATFORM_DE_INIT) {
		pr_err("DPM: %s: Need to free resoruce in the future\n", __func__);
		return DP_SUCCESS;
	}
	cpu_port->alloc_flags = DP_F_CPU;

	if (platform_map_to_drop_q(inst)) {
		pr_err("DPM: %s: platform_map_to_drop_q failed\n", __func__);
		return DP_FAILURE;
	}

	if (cpu_mode_table_cfg(inst)) {
		pr_err("DPM: %s: cpu_mode_table_cfg failed\n", __func__);
		return DP_FAILURE;
	}

	cpu_port->status = PORT_SUBIF_REGISTERED;

	p = dp_kzalloc(sizeof(*p), GFP_ATOMIC);
	if (!p)
		return DP_FAILURE;

	if (alloc_cqm_cpu_port_data(&p->cpu_data))
		goto exit;

	/* Alloc queue/scheduler/port per CPU port */
	p->cpu_data.dp_inst = inst;
	p->cpu_data.cqm_inst = cqm_inst;

	if (is_soc_tpz(inst))
		flags = 1;

	if (dp_cqm_cpu_port_get(inst, &p->cpu_data, flags)) {
		pr_err("DPM: %s fail for CPU Port. Why ???\n", "cqm_cpu_port_get");
		goto exit;
	}
	dump_cpu_data(inst, &p->cpu_data);

	cpu_port->rx_cpu_rmap = p->cpu_data.rmap;

	if (dev_platform_set(inst, CPU_PORT, NULL, 0)) {
		pr_err("DPM: %s fail for CPU Port. at dev_platform_set()\n", __func__);
		goto exit;
	}
	if (is_soc_tpz(inst)) {
		/* update gpid status table */
		for (i = 0; i < cpu_port->num_gpid; i++) {
			vap = DP_DPID0_CPU_SUBIF_START + i;
			priv->gp_dp_map[cpu_port->gpid[i]].subif =
				SET_VAP(vap, cpu_port->vap_offset,
					cpu_port->vap_mask);
			get_dp_port_subif(cpu_port, vap)->gpid =
				cpu_port->gpid[i];
		}
	}

	for (i = 0; i < cpu_port->max_subif; i++) {
		for (j = 0; j < DP_DFL_SESS_NUM; j++) {
			subif_info = get_dp_port_subif(cpu_port, i);
			subif_info->dfl_eg_sess[0][j] = -1;
		}
	}

	dp_cpu_init_ok = 1;
	cap = &get_dp_prop_info(inst)->cap;
	if (cap->max_cpu > CQM_MAX_CPU)
		cap->max_cpu = CQM_MAX_CPU;
	if (cap->max_port_per_cpu > MAX_PORTS_PER_CPU)
		cap->max_port_per_cpu = MAX_PORTS_PER_CPU;
	/* For PRX Max CPU 4 and max_port_per_cpu is 1
	 * For LGM Max CPU 4 and max_port_per_cpu is 2
	 * For TPZ Maz CPU 4 and max_deq_ring_per_cpu is 2
	 */
	cpu_port->num_deq = 0;
	deq_idx = 0;

	for (cpu_id = 0; cpu_id < cap->max_cpu; cpu_id++) {
		need_netdev = false;
		vap = DP_DPID0_CPU_SUBIF_START + cpu_id;
		subif_info = get_dp_port_subif(cpu_port, vap);

		for (cpu_port_num = 0; cpu_port_num < cap->max_port_per_cpu;
		     cpu_port_num++,deq_idx++) {
			if (p->cpu_data.cpu_deq_info[cpu_id][cpu_port_num].attr.ring.index < 0) {
				/* workaround here to make cat dp/qos work */
				cpu_port->num_deq++;
				cpu_port->deq[deq_idx]->attr.ring.index = -1;
				cpu_port->deq[deq_idx]->attr.ring.port_id = -1;
				continue;
			}
			deq = get_dp_deqring_info(inst,
					p->cpu_data.cpu_deq_info[cpu_id][cpu_port_num].attr.ring.index);
			cpu_port->deq[deq_idx] = &deq->deq;
			deq->dts_qos = dp_get_qos_cfg(inst, CPU_PORT, DP_F_CPU, 0);
			cpu_port->num_deq++;
			deq->type = CQM_DEQ_RING_NORM;

			res = alloc_cpu_q(inst, &p->cpu_data, &p->q_port, cpu_id,
					   cpu_port_num, 0);
			if (res > 0) {
				/* cqm dequeue port is not initialized, ie,
				 * those CPU is offline
				 */
				cpu_port->deq[deq_idx]->attr.ring.index = -1;
				cpu_port->deq[deq_idx]->attr.ring.port_id = -1;
				cpu_port->num_deq--;
				continue;
			} else if (res < 0)
				goto exit;
			save_cpu_subif_info(inst, &p->cpu_data, &p->q_port,
					    cpu_port, cpu_id, cpu_port_num,
					    vap, subif_info);
			deq->q_node = p->q_port.q_node;
			if (!cpu_port_num) {
				if (dp_add_pp_gpid(inst, CPU_PORT, vap,
							get_dp_port_subif(cpu_port, vap)->gpid,
							0, 0)) {
					pr_err("DPM: dp_alloc_pp_gpid fail for CPU VAP=%d\n",
							vap);
					goto exit;
				}
			}
			/* Map all CPU port's lookup to one of default CPU's
			 * 1st queue only
			 */
			subif_info = get_dp_port_subif(cpu_port, vap);
			subif_info->port_info = cpu_port;
			subif_info->dp_port = CPU_PORT;
			subif_info->subif_groupid = vap;
			if (!f_cpu_q && cpu_id == p->cpu_data.default_cpu) {
				/* only run 1 times per DP instance herre */
				f_cpu_q = 1;
				dflt_q_cpu[inst] = p->q_port.qid;
				dflt_cpu_vap[inst] = vap;
				_dp_init_subif_q_map_rules(subif_info, 0);
				hostif.inst = inst;
				hostif.gpid = get_dp_port_subif(cpu_port, vap)->gpid;
				hostif.qid = p->q_port.qid;
				hostif.color = PP_COLOR_GREEN;
				if (dp_add_dflt_hostif(&hostif, 0)) {
					pr_err("DPM: %s fail for CPU VAP=%d\n",
					       "dp_add_dflt_hostif", vap);
					goto exit;
				}
			}
			p->en_data.cqm_inst = cqm_inst;
			p->en_data.dp_inst = inst;
			p->en_data.deq_port_ring[0] = p->q_port.cqe_deq;
			p->en_data.num_deq_ring = 1;
			/*TODO: VBOLLA: This can be called one time, by filling all info once*/
			if (dp_enable_cqm_rings(NULL, get_dp_port_info(inst, CPU_PORT),
				    &p->en_data, 0)) {
				pr_err("DPM: %s: Fail to enable CPU[%d]\n",
				       __func__, p->en_data.deq_port_ring[0]);
				goto exit;
			}
			need_netdev = true;
		}
		if (need_netdev)
			dp_cpu_netdev(inst, cpu_id, vap, flag);
		
		/*increase num subif*/
		cpu_port->num_subif++;
	}

	/* set first cpu queue map if needed
	 * In fact, CPU queue mapping is only for FLM, not LGM since LGM's CPU
	 * queue mapping rule is empty
	 */
	subif_info = get_dp_port_subif(cpu_port, dflt_cpu_vap[inst]);
	_dp_set_subif_q_lookup_tbl(subif_info, 0, 0);
	ret = DP_SUCCESS;
exit:
	free_cqm_cpu_port_data(&p->cpu_data);
	kfree(p);
	return ret;
}

void dump_hal_priv_info(int inst, struct hal_priv *priv)
{
	DP_DUMP("--hal_priv info[%d]--\n", inst);
	DP_DUMP("deq_ring_stat =%lx\n", (unsigned long) priv->deq_ring_stat);
	DP_DUMP("qos_queue_stat=%lx\n", (unsigned long) priv->qos_queue_stat);
	DP_DUMP("qos_sch_stat  =%lx\n", (unsigned long) priv->qos_sch_stat);
	DP_DUMP("resv          =%lx\n", (unsigned long) priv->resv);
}

static int dp_platform_reset(int inst, u32 flag)
{
	struct hal_priv *priv;
	struct dp_port_info *pi = get_dp_port_info(inst, CPU_PORT);
	struct inst_property *dp_prop = get_dp_port_prop(inst);
	int i;

	dev_platform_set(inst, CPU_PORT, NULL, DP_F_DEREGISTER);

	priv = (struct hal_priv *)dp_prop->priv_hal;
	dp_platform_queue_set(inst, flag);
	init_ppv4_qos(inst, flag); /* de-initialize qos */
	for (i = 0; i < pi->num_gpid; i++) {
		free_gpid(inst, pi->gpid[i], 1, pi->gpid_spl);
		pi->gpid_spl = -1;
	}
	pi->num_gpid = 0;

	if (!inst) {
		dp_sub_proc_uninstall();
	}

	kfree(dp_prop->priv_hal);
	dp_prop->priv_hal = NULL;
	return DP_SUCCESS;
}

static int dp_platform_set(int inst, u32 flag)
{
#if 0 //GSW_ENABLE
	GSW_QoS_portRemarkingCfg_t *port_remark = NULL;
	struct core_ops *gsw_ops;
	struct qos_ops *gsw_qos;
#endif
	struct hal_priv *priv;
	struct dp_port_info *pi = get_dp_port_info(inst, CPU_PORT);
	struct inst_property *dp_prop = get_dp_port_prop(inst);
	struct dp_subif_info *sif;
	int i;

	if (flag & DP_PLATFORM_DE_INIT) /* de-initialize */
		return dp_platform_reset(inst, flag);

	/* For initialize */
	dp_prop->priv_hal = dp_kzalloc(sizeof(*priv), GFP_ATOMIC);
	if (ZERO_OR_NULL_PTR(dp_prop->priv_hal))
		return DP_FAILURE;
	priv = (struct hal_priv *)dp_prop->priv_hal;
	priv->inst = inst;
	/* Set CQM deq ring initial status to disbled in our DB*/
	for (i = 0; i < DP_MAX_PPV4_PORT; i++) {
		priv->deq_ring_stat[i].disabled = 1;
	}

#if 0 //GSW_ENABLE
	gsw_ops = dp_prop->ops[0];
	gsw_qos = &gsw_ops->gsw_qos_ops;
#endif
	if (!inst) {
		/*only inst zero need ADP descriptor */
		init_adp_template(CPU_PORT, flag, false);
		dp_sub_proc_install();
	}

	sif = get_dp_port_subif(pi, CPU_SUBIF);
	pi->alloc_flags = DP_F_CPU;
	pi->inst_prop = get_dp_port_prop(inst);
	pi->inst = inst;
#if 0 //GSW_ENABLE
	priv->bp_def = dp_gsw_alloc_bp(inst, CPU_PORT, CPU_SUBIF,
				       CPU_FID, CPU_BP, 0);
	if (priv->bp_def < 0) {
		pr_err("DPM: %s: dp_gsw_alloc_bp failed\n", __func__);
		goto ERROR;
	}
	if (dp_gsw_get_parser(NULL, NULL, NULL, NULL)) {
		pr_err("DPM: %s: dp_get_gsw_parser fail\n", __func__);
		goto ERROR;
	}
	port_remark = dp_kzalloc(sizeof(*port_remark), GFP_ATOMIC);
	if (!port_remark)
		goto ERROR;
	/* disable egress VLAN modification for CPU port */
	port_remark->nPortId = 0;
	if (gsw_qos->QoS_PortRemarkingCfgGet(gsw_ops, port_remark)) {
		pr_err("DPM: %s: GSW_QOS_PORT_REMARKING_CFG_GET failed\n",
		       __func__);
		goto ERROR;
	}

	port_remark->bPCP_EgressRemarkingEnable = 0;
	if (gsw_qos->QoS_PortRemarkingCfgSet(gsw_ops, port_remark)) {
		pr_err("DPM: %s: GSW_QOS_PORT_REMARKING_CFG_SET failed\n",
		       __func__);
		goto ERROR;
	}
#endif
	if (init_ppv4_qos(inst, flag)) {
		pr_err("DPM: %s: init_ppv4_qos fail\n", __func__);
		goto ERROR;
	}

	if (dp_platform_queue_set(inst, flag)) {
		pr_err("DPM: %s: dp_platform_queue_set fail\n", __func__);
		goto ERROR;
	}

#if 0 //GSW_ENABLE
	if (dp_gsw_color_table_set(inst)) {
		pr_err("DPM: %s: dp_gsw_color_table_set Failed\n",
		       __func__);
		goto ERROR;
	}

	if (dp_gsw_dis_cpu_vlan_md(inst)) {
		pr_err("DPM: %s: dp_gsw_dis_cpu_vlan_md fail\n", __func__);
		goto ERROR;
	}
#endif

	if (dp_tx_init(inst)) {
		pr_err("DPM: %s: dp_tx_init fail\n", __func__);
		goto ERROR;
	}

	/* Initialize sererved GPID<->LPID mapping in gp_dp_map table */
	dp_init_reserved_gpid_map(inst);

	/* Explicitely configure reserved GPID<->LPID mapping in CQM.
	 * GSWIP HW already has this mapping, so no need to configure
	 * for GSWIP
	 */
	if (dp_cqm_set_reserved_gpid_map(inst)) {
		pr_err("DPM: %s: dp_cqm_set_default_gpid_map failed\n",
		       __func__);
		goto ERROR;
	}
#if 0 //GSW_ENABLE
	kfree(port_remark);
#endif
	return DP_SUCCESS;

ERROR:
	if (dp_prop->priv_hal) {
		kfree(dp_prop->priv_hal);
		dp_prop->priv_hal = NULL;
	}
#if 0 //GSW_ENABLE
	if (port_remark)
		kfree(port_remark);
#endif
	return DP_FAILURE;

}

/* API to enable GSWIP PCE processing for PON port
 *
 * Note - Later if need to consider de-registration & reset of GSWIP
 * Qos_QueuePort table, store original qid information to DPM port info
 * using traffic class 0 qid
 */
static int __maybe_unused dp_gsw_pce_enable_pon(int inst, int ep, u32 flags)
{
#if 0 //GSW_ENABLE
	struct qos_ops *gsw_qos;
	GSW_QoS_queuePort_t *q_map;
	int i = 0;
	struct core_ops *gsw_ops = dp_port_prop[inst].ops[0];

	gsw_qos = &gsw_ops->gsw_qos_ops;

	q_map = dp_kzalloc(sizeof(*q_map), GFP_ATOMIC);
	if (!q_map)
		return DP_FAILURE;
	/* Disable Ingress PCE Bypass for Port (PON/Eth) TC 0 .. 15 */
	for (i = 0; i <= 15; i++) {
		q_map->nPortId = ep;
		q_map->nTrafficClassId = i;
		if (gsw_qos->QoS_QueuePortGet(gsw_ops, q_map)) {
			kfree(q_map);
			pr_err("DPM: %s: Fail in QoS_QueuePortGet\n", __func__);
			return DP_FAILURE;
		}
		if (!(flags & DP_F_DEREGISTER))
			q_map->bEnableIngressPceBypass = 0;
		else {
			q_map->bEnableIngressPceBypass = 1;
			q_map->nRedirectPortId = PMAC_2;
		}
		if (gsw_qos->QoS_QueuePortSet(gsw_ops, q_map)) {
			kfree(q_map);
			pr_err("DPM: %s: Fail in QoS_QueuePortSet\n", __func__);
			return DP_FAILURE;
		}
	}
	kfree(q_map);
#endif
	return DP_SUCCESS;
}

/* API to redirect packet to PMAC 1 in gswip second loop for PON HGU */
static int __maybe_unused dp_gsw_secloop_cfg(int inst, int ep, u32 flags)
{
#if 0 //GSW_ENABLE
	struct qos_ops *gsw_qos;
	GSW_QoS_queuePort_t *q_map;
	int i = 0;
	struct core_ops *gsw_ops = dp_port_prop[inst].ops[0];
	struct pon_spl_cfg *cfg = &pon_cfg[0];

	for (i = 0; i < ARRAY_SIZE(pon_cfg); i++) {
		/* will handle de_register here according to flags */
		if ((pon_cfg[i].flag & flags) == flags) {
			cfg = &pon_cfg[i];
			break;
		}
	}

	gsw_qos = &gsw_ops->gsw_qos_ops;

	q_map = dp_kzalloc(sizeof(*q_map), GFP_ATOMIC);
	if (!q_map)
		return DP_FAILURE;
	/* EP=2 remapped to Q 28 .. Q 31 and redirect to Pmac 1 */
	/* if PCE is bypassed, Q is based on reduced traffic class 2 bits
	 * if PCE is not bypassed which is the GSWIP second loop case, traffic
	 * class is 4 bits
	 */
	for (i = 0; i <= 15; i++) {
		q_map->nPortId = ep;
		q_map->nTrafficClassId = i;
		if (gsw_qos->QoS_QueuePortGet(gsw_ops, q_map)) {
			kfree(q_map);
			pr_err("DPM: %s: Fail in QoS_QueuePortGet\n", __func__);
			return DP_FAILURE;
		}

		/* Currently QID is hardcoded, later when
		 * GSWIP default config is ready QID can get from GSWIP
		 * Qos Queue port Get API
		 * traffic class 0 - 3 use Q28 to Q31, and 4 - 15 use Q31 all
		 * redirect to PMAC 1
		 */
		if (i >= 3)
			q_map->nQueueId = cfg->qid + 3;
		else
			q_map->nQueueId = cfg->qid + i;

		if (flags & DP_F_DEREGISTER)
			q_map->nQueueId = cfg->qid;

		q_map->nRedirectPortId = cfg->pmac;
		if (gsw_qos->QoS_QueuePortSet(gsw_ops, q_map)) {
			kfree(q_map);
			pr_err("DPM: %s: Fail in QoS_QueuePortSet\n", __func__);
			return DP_FAILURE;
		}
	}
	kfree(q_map);
#endif
	return DP_SUCCESS;
}

static int __maybe_unused pon_config(int inst, int ep, struct dp_port_data *data,
		      u32 flags)
{
	struct pon_spl_cfg *cfg = &pon_cfg[0];
	(void)cfg;
#if 0 //GSW_ENABLE need to adpat to IPPU/EPPU
	struct core_ops *gsw_ops;
	struct common_ops *gsw_com;
	struct mac_ops *mac_ops;
	GSW_CPU_PortCfg_t *cpu_port_cfg;
	int i;

	for (i = 0; i < ARRAY_SIZE(pon_cfg); i++) {
		/* will handle de_register here according to flags */
		if ((pon_cfg[i].flag & flags) == flags) {
			cfg = &pon_cfg[i];
			break;
		}
	}

	mac_ops = dp_port_prop[inst].mac_ops[ep];
	gsw_ops = dp_port_prop[inst].ops[GSWIP_L];
	gsw_com = &gsw_ops->gsw_common_ops;

	cpu_port_cfg = dp_kzalloc(sizeof(*cpu_port_cfg), GFP_ATOMIC);
	if (!cpu_port_cfg)
		return DP_FAILURE;

	if (gsw_com->CPU_PortCfgGet(gsw_ops, cpu_port_cfg)) {
		kfree(cpu_port_cfg);
		pr_err("DPM: %s: fail in getting CPU port config\n", __func__);
		return DP_FAILURE;
	}
	/* Setting Egress and Ingress Special Tag */
	cpu_port_cfg->nPortId = ep;
	cpu_port_cfg->bSpecialTagIngress = cfg->ingress;
	cpu_port_cfg->bSpecialTagEgress = cfg->egress;
	if (gsw_com->CPU_PortCfgSet(gsw_ops, cpu_port_cfg)) {
		kfree(cpu_port_cfg);
		pr_err("DPM: %s: Fail in configuring CPU port\n", __func__);
		return DP_FAILURE;
	}

	if (is_soc_tpz(inst)) {
		if (dp_gsw_pce_enable_pon(inst, ep, flags)) {
			kfree(cpu_port_cfg);
			return DP_FAILURE;
		}
		if (dp_gsw_secloop_cfg(inst, ep, flags)) {
			kfree(cpu_port_cfg);
			return DP_FAILURE;
		}
	}

	/* Rx CRC check. Value '0'-enable, '1'-disable */
	mac_ops->set_rx_crccheck(mac_ops, cfg->crc_check);

	/* TX FCS generation*/
	if (data->flag_ops & DP_F_DATA_FCS_DISABLE)
		mac_ops->set_fcsgen(mac_ops, cfg->fcs_gen);

	/* RX/TX Flow control */
	mac_ops->set_flow_ctl(mac_ops, cfg->flow_ctrl);

	/* Replace Tx Special Tag for PON registration */
	mac_ops->mac_op_cfg(mac_ops, cfg->tx_spl_tag);

	/* Indicate GSWIP that packet coming from PON have timestamp
	 * In acceleration path, GSWIP can remove the timestamp
	 * during registration
	 */
	mac_ops->mac_op_cfg(mac_ops, cfg->rx_time_stamp);

	/* PON Interface always have a Special Tag from PON -> Xgmac
	 * so should disable the Dummy Special Tag for PON registration
	 */
	mac_ops->mac_op_cfg(mac_ops, cfg->rx_spl_tag);

	/* If PON IP keeps the FCS towards the SoC then the MAC
	 * should not remove the FCS. The FCS is then removed by
	 * the FDMA and FCS is recalculated if packet was modified
	 * in thus way.
	 */
	if (data->flag_ops & DP_F_REM_FCS) {
		mac_ops->mac_op_cfg(mac_ops, RX_FCS_NO_REMOVE);
		mac_ops->mac_op_cfg(mac_ops, TX_FCS_REMOVE);
	}

	if (flags & DP_F_DEREGISTER)
		mac_ops->mac_op_cfg(mac_ops, cfg->rx_fcs);

	/* Reset the MAC, without this reset the downstream from the PON IP
	 * will not work when the MAC is not reset in U-Boot before.
	 */
	mac_ops->soft_restart(mac_ops);

	/* Force the link to the PON IP into up state.
	 * The XGMAC on LGM does not auto detect that it is up.
	 * Without this no traffic or OMCI will pass to the XGAMC.
	 */
	mac_ops->set_link_sts(mac_ops, cfg->link_sts);

	kfree(cpu_port_cfg);
#endif
	return DP_SUCCESS;
}

/* Generic API for GSWIP ingress PCE config
 * For now using cfg only for VUNI port
 * if IGP=vUNI/vANI Ingress PCE Bypass disable
 * and if EGP=vUNI/vANI redirect to Pmac_2 to PPv4
 */
static int __maybe_unused dp_gsw_pce_enable(int inst, int ep, struct dp_port_data *data,
			     u32 flags)
{
#if 0 //GSW_ENABLE
	struct core_ops *gsw_ops;
	struct qos_ops *gsw_qos;
	int j = 0;
	GSW_QoS_queuePort_t *q_map;
	bool ig_bypass = false;
	u32 qid = 0;

	gsw_ops = dp_port_prop[inst].ops[GSWIP_L];
	gsw_qos = &gsw_ops->gsw_qos_ops;

	/* Disable Ingress PCE Bypass for vUNI TC 0 .. 15
	 * EP=12 remapped to Q 12 .. Q 15 and redirect to Pmac 2 and to PPv4
	 */
	q_map = dp_kzalloc(sizeof(*q_map), GFP_ATOMIC);
	if (!q_map)
		return DP_FAILURE;

	/* De-register */
	if (flags & DP_F_DEREGISTER)
		ig_bypass = true;

	for (j = 0; j <= 15; j++) {
		q_map->nPortId = ep;
		q_map->nTrafficClassId = j;
		if (gsw_qos->QoS_QueuePortGet(gsw_ops, q_map)) {
			kfree(q_map);
			pr_err("DPM: %s: Fail in QoS_QueuePortGet\n", __func__);
			return DP_FAILURE;
		}

		q_map->bEnableIngressPceBypass = ig_bypass;
		q_map->nRedirectPortId = PMAC_2;
		if (j == 0)
			qid = q_map->nQueueId;
		else
			q_map->nQueueId = qid;
		if (gsw_qos->QoS_QueuePortSet(gsw_ops, q_map)) {
			kfree(q_map);
			pr_err("DPM: %s: Fail in QoS_QueuePortSet\n", __func__);
			return DP_FAILURE;
		}
	}
	kfree(q_map);
#endif
	return DP_SUCCESS;
}

//TODO: VBOLLA: static int dp_port_spl_cfg(int inst, int dpid, struct dp_port_data *data,
//TODO: VBOLLA: 			   u32 flags)
//TODO: VBOLLA: {
//TODO: VBOLLA: 	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
//TODO: VBOLLA: 
//TODO: VBOLLA: 	if (is_xpon(port_info->alloc_flags))
//TODO: VBOLLA: 		pon_config(inst, ep, data, flags);
//TODO: VBOLLA: 	else if (is_soc_tpz(inst) && port_info->alloc_flags & DP_F_VUNI)
//TODO: VBOLLA: 		dp_gsw_pce_enable(inst, ep, data, flags);
//TODO: VBOLLA: 
//TODO: VBOLLA: 	if (is_soc_tpz(inst) && (data->flag_ops & DP_F_DATA_PON_HGU_SEC_LOOP)) {
//TODO: VBOLLA: 		if (dp_gsw_secloop_cfg(inst, ep, flags))
//TODO: VBOLLA: 			return DP_FAILURE;
//TODO: VBOLLA: 	}
//TODO: VBOLLA: 
//TODO: VBOLLA: 	return DP_SUCCESS;
//TODO: VBOLLA: }

static int port_platform_reset(int inst, u8 dpid,
					struct dp_port_data *data, u32 flags)
{
	struct dp_port_info *port_info = get_dp_port_info(inst, dpid);
	struct cqm_deq_ring_info *deq_rinfo;
	int i, ring_idx;

	//VBOLLA: dp_port_spl_cfg(inst, ep, data, flags);

	if ((port_info->alloc_flags & DP_F_ACA))
		return DP_SUCCESS;

	/* need reset cqm_port_info if no one is using it any more.
	 * For ACA case, it is done during dp_deregister_dev stage via rx ring
	 * So here only handle non-ACA case
	 */
	for (i = 0; i < port_info->num_deq; i++) {
		ring_idx = port_info->deq[i]->attr.ring.index;
		deq_rinfo = get_dp_deqring_info(inst, ring_idx);
		if (deq_rinfo->ref_cnt)
			continue;
		dp_memset(deq_rinfo, 0, sizeof(*deq_rinfo));
	}

	return DP_SUCCESS;
}

static int port_platform_set(int inst, u8 dpid, struct dp_port_data *data,
			     u32 flags)
{
	struct hal_priv *priv = (struct hal_priv *)dp_port_prop[inst].priv_hal;

	if (!priv) {
		pr_err("DPM: %s: priv is NULL\n", __func__);
		return DP_FAILURE;
	}
	if (flags & DP_F_DEREGISTER)
		return port_platform_reset(inst, dpid, data, flags);

	//TODO: VBOLLA: dp_gsw_set_port_lu_md(inst, ep, flags);
	DP_DEBUG(DP_DBG_FLAG_QOS, "inst: %d, priv=0x%px deq_ring_stat=0x%px qdev=0x%px\n",
		 inst, priv,
		 priv ? priv->deq_ring_stat : NULL,
		 priv ? priv->qdev : NULL);

	//VBOLLA: dp_port_spl_cfg(inst, dpid, data, flags); TODO: VBOLLA: need to check
	//with guohua

//VBOLLA: #if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
//VBOLLA: 	if (DP_DBG_FLAG_QOS & dp_dbg_flag) {
//VBOLLA: 		for (i = 0; i < port_info->num_deq; i++) {
//VBOLLA: 			deq_pinfo = get_dp_deqport_info(inst,
//VBOLLA: 				port_info->deq_ports[i]);
//VBOLLA: 			DP_DUMP("   CQM deq_port[%-3d]: %s = %2d, %s = %3d, %s = 0x%px,\n"
//VBOLLA: 				"%-22s%s = %2d, %s = %3d, %s = 0x%px,\n"
//VBOLLA: 				"%-22s%s = %2d\n",
//VBOLLA: 				port_info->deq_ports[i],
//VBOLLA: 				"f_dma_ch  ", deq_pinfo->f_dma_ch,
//VBOLLA: 				"tx_pkt_credit", deq_pinfo->tx_pkt_credit,
//VBOLLA: 				"tx_push ", deq_pinfo->txpush_addr,
//VBOLLA: 				"", "dma_ch_off", deq_pinfo->dma_ch_offset,
//VBOLLA: 				"tx_ring_size ", deq_pinfo->tx_ring_size,
//VBOLLA: 				"qos_push", deq_pinfo->txpush_addr_qos,
//VBOLLA: 				"", "dma_chan  ", deq_pinfo->dma_chan);
//VBOLLA: 		}
//VBOLLA: 	}
//VBOLLA: #endif
	return DP_SUCCESS;
}

static int __maybe_unused set_ctp_bp(int inst, int ctp, int portid, int bp,
		      struct subif_platform_data *data)
{
#if 0 //GSW_ENABLE
	GSW_CTP_portConfig_t *ctp_cfg;
	struct core_ops *gsw_ops;
	struct inst_info *i_info;
	struct dp_port_info *port_info;

	ctp_cfg = dp_kzalloc(sizeof(*ctp_cfg), GFP_ATOMIC);
	if (!ctp_cfg)
		return DP_FAILURE;
	port_info = get_dp_port_info(inst, portid);
	i_info = &dp_port_prop[inst].info;
	gsw_ops = dp_port_prop[inst].ops[GSWIP_L];

	ctp_cfg->nLogicalPortId = portid;
	ctp_cfg->nSubIfIdGroup = ctp;
	ctp_cfg->eMask = GSW_CTP_PORT_CONFIG_MASK_BRIDGE_PORT_ID;
	ctp_cfg->nBridgePortId = bp;
	if (gsw_ops->gsw_ctp_ops.CTP_PortConfigSet(gsw_ops, ctp_cfg)) {
		pr_err("DPM: %s: Failed to CTP(%d)'s bridge port=%d for ep=%d\n",
		       __func__, ctp, bp, portid);
		kfree(ctp_cfg);
		return DP_FAILURE;
	}

	if (!data || !data->subif_data) {
		pr_err("DPM: %s: data NULL or subif_data NULL\n", __func__);
		kfree(ctp_cfg);
		return DP_SUCCESS;
	}

	if (!data->subif_data->ctp_dev) {
		kfree(ctp_cfg);
		return DP_SUCCESS;
	}

	/* Copy first flow entry index incase of VLAN aware Pmapper dev */
	if (dp_gsw_copy_ctp_cfg(inst, bp, NULL,	portid)) {
		DP_DEBUG(DP_DBG_FLAG_REG,
			 "%s: gsw_ctp_cfg copy bp=%d not required\n",
			 __func__, bp);
		kfree(ctp_cfg);
		return DP_SUCCESS;
	}
	kfree(ctp_cfg);
#endif
	return DP_SUCCESS;
}

static int __maybe_unused reset_ctp_bp(int inst, int ctp, int portid, int bp)
{
#if 0 //GSW_ENABLE
	GSW_CTP_portConfig_t *ctp_cfg;
	struct core_ops *gsw_ops = dp_port_prop[inst].ops[GSWIP_L];
	struct dp_port_info *port_info;
	struct dp_subif_info *sif;
	struct hal_priv *priv = (struct hal_priv *)dp_port_prop[inst].priv_hal;

	ctp_cfg = dp_kzalloc(sizeof(*ctp_cfg), GFP_ATOMIC);
	if (!ctp_cfg)
		return DP_FAILURE;
	port_info = get_dp_port_info(inst, portid);
	sif = get_dp_port_subif(port_info, ctp);

	ctp_cfg->nLogicalPortId = portid;
	ctp_cfg->nSubIfIdGroup = ctp;
	ctp_cfg->nBridgePortId = priv->bp_def; /* change back to bp_def */
	ctp_cfg->eMask = GSW_CTP_PORT_CONFIG_MASK_BRIDGE_PORT_ID;
	if (gsw_ops->gsw_ctp_ops.CTP_PortConfigSet(gsw_ops, ctp_cfg)) {
		pr_err("DPM: %s: Failed to reset CTP(%d)'s bridge port=%d for ep=%d\n",
		       __func__, ctp, bp, portid);
		kfree(ctp_cfg);
		return DP_FAILURE;
	}
	kfree(ctp_cfg);
#endif
	return DP_SUCCESS;
}

static
int alloc_q(int inst, struct ppv4_q_sch_port *q_port,
	    struct cqm_deq_ring_info *deq_rinfo)
{
	struct q_info *q_info;

	DP_DEBUG(DP_DBG_FLAG_QOS,
		 "Queue decision: %s\n", "auto_new_queue");

	if (alloc_q_to_port(q_port, 0)) {
		pr_err("DPM: %s: %s fail for dp_port=%d\n",
		       __func__, "alloc_q_to_port", q_port->dp_port);
		return DP_FAILURE;
	}

	q_info = get_dp_q_info(inst, q_port->qid);
	if (q_info->flag) {
		pr_err("DPM: %s: Why dp_q_tbl[%d][%d].flag =%d%s?\n",
		       __func__, inst, q_port->qid, q_info->flag, ":expect 0");
		return DP_FAILURE;
	}

	if (q_info->ref_cnt) {
		pr_err("DPM: %s: Why dp_q_tbl[%d][%d].ref_cnt =%d%s?\n",
		       __func__, inst, q_port->qid, q_info->ref_cnt,
		       ":expect 0");
		return DP_FAILURE;
	}

	/* update queue table */
	q_info->flag = 1;
	q_info->need_free = 1;
	q_info->ref_cnt = 1;
	q_info->q_node_id = q_port->q_node;
	q_info->cqm_dequeue_ring = q_port->cqe_deq;

	deq_rinfo->qid = q_port->qid;

	DP_DEBUG(DP_DBG_FLAG_REG,
		 "%s: %s=%d %s=%d q[%d].cnt=%d\n",
		 "new_queue_alloc",
		 "dp_port", q_port->dp_port,
		 "vap", q_port->vap,
		 q_port->qid, get_dp_q_info(inst, q_port->qid)->ref_cnt);

	return DP_SUCCESS;
}

static
int free_q(int inst, int dp_port, int qid, struct cqm_deq_ring_info *deq_rinfo,
	      int cqm_deq_ring)
{
	struct q_info *q_info;
	struct dp_node_alloc *node;

	q_info = get_dp_q_info(inst, qid);
	if (!q_info->need_free)
		return DP_FAILURE;

	if (!q_info->ref_cnt) {
		pr_err("DPM: %s: Why dp_q_tbl[%d][%d].ref_cnt Zero:expect > 0\n",
		       __func__, inst, qid);
		return DP_FAILURE;
	}
	node = dp_kzalloc(sizeof(*node), GFP_ATOMIC);
	if (!node)
		return DP_FAILURE;

	q_info->ref_cnt--;
	if (q_info->flag && !q_info->ref_cnt) {
		DP_DEBUG(DP_DBG_FLAG_REG, "Free qid %d\n", qid);
		node->id.q_id = qid;
		/*if no subif using this queue, need to delete it*/
		node->inst = inst;
		node->dp_port = dp_port;
		node->type = DP_NODE_QUEUE;
		dp_node_free(node, 0);

		/*update dp_q_tbl*/
		q_info->flag = 0;
		q_info->need_free = 0;
	} else {
		kfree(node);
		DP_DEBUG(DP_DBG_FLAG_QOS, "q_id[%d] dont need to be freed\n",
			 qid);
		return DP_FAILURE;
	}

	if (deq_rinfo->qid == qid) {
		/*Assign QID to default value -1*/
		deq_rinfo->qid = -1;
		DP_DEBUG(DP_DBG_FLAG_QOS, "q_id[%d] is freed\n", qid);
	}
	if (!deq_rinfo->ref_cnt) {
		node->id.cqm_deq_ring = cqm_deq_ring;
		/*if no subif using this queue, need to delete it*/
		node->inst = inst;
		node->dp_port = dp_port;
		node->type = DP_NODE_RING;
		dp_node_free(node, 0);
		DP_DEBUG(DP_DBG_FLAG_QOS,
			 "cqm_ring[%d] is freed\n", cqm_deq_ring);
	} else {
		DP_DEBUG(DP_DBG_FLAG_QOS,
			 "cqm_ring[%d] dont need to be freed\n", cqm_deq_ring);
	}

	kfree(node);
	return DP_SUCCESS;
}

/*
 * Allocate one TX IPPU queue if not already allocated
 * can be shared, so always check q_info->ref_cnt
 */
static int dp_alloc_tx_ippu_q(int inst, int portid, int subif_ix)
{
	struct dp_ring_pktdeq *dp_port_deq_rinfo;
	struct cqm_deq_ring_info *deq_rinfo;
	struct hal_priv *priv = HAL(inst);
	struct ppv4_q_sch_port *q_port;
	struct dp_port_info *port_info;
	struct dp_subif_info *sif;
	struct q_info *q_info;
	int ret = DP_FAILURE;
	int i;

	q_port = dp_kzalloc(sizeof(*q_port), GFP_ATOMIC);
	if (!q_port)
		return DP_FAILURE;

	port_info = get_dp_port_info(inst, portid);
	sif = get_dp_port_subif(port_info, subif_ix);

	for (i = 0; i < port_info->num_tx_ippu; i++) {
		dp_port_deq_rinfo = port_info->tx_ippu[i];
		q_port->cqe_deq = dp_port_deq_rinfo->attr.ring.index;

		_DP_DEBUG(DP_DBG_FLAG_QOS, "DPM: %s: %s=%d %s=0x%px %s=0x%px %s=0x%px %s=%d\n",
			 __func__,
			 "cqe_txippu", q_port->cqe_deq,
			 "priv", priv,
			 "deq_ring_stat", priv ? priv->deq_ring_stat : NULL,
			 "qdev", priv ? priv->qdev : NULL,
			 "inst", inst);
		deq_rinfo = get_dp_deqring_info(inst, q_port->cqe_deq);
		if (deq_rinfo->ref_cnt) {
			/*Queue for this IPPU ring is already allocated*/
			q_port->qid = deq_rinfo->qid;
			q_info = get_dp_q_info(inst, q_port->qid);
			q_port->q_node = q_info->q_node_id;
			if(!q_info->ref_cnt)
				pr_err("DPM: %s, something wrong, tx ippu deq->refcnt: %d and q->refcnt: %d not aligned\n",
						__func__, deq_rinfo->ref_cnt, q_info->ref_cnt);
			q_info->ref_cnt++;
			q_port->port_node = priv->deq_ring_stat[q_port->cqe_deq].node_id;
		} else {
			q_port->tx_pkt_credit = deq_rinfo->deq.pkt_credit;
			q_port->tx_ring_addr = deq_rinfo->deq.txpush_addr_qos;
			q_port->tx_ring_addr_push = deq_rinfo->deq.attr.paddr;
			q_port->tx_ring_size = deq_rinfo->deq.attr.size;
			q_port->inst = inst;
			q_port->dp_port = portid;
			q_port->vap = subif_ix;
			if (alloc_q(inst, q_port, deq_rinfo))
				goto free;
		}
		deq_rinfo->ref_cnt++;

		/* update ippu qid into subif table */
		sif->tx_ippu_qid_list[i] = q_port->qid;
		sif->tx_ippu_q_node[i] = q_port->q_node;
		sif->tx_ippu_qos_deq_port[i] = q_port->port_node;
		sif->tx_ippu_ring[i] = q_port->cqe_deq;
		sif->num_tx_ippu++;
	}
	ret = DP_SUCCESS;
free:
	kfree(q_port);
	return ret;
}

/*
 * Allocate one RX IPPU queue, if not already allocated
 * can be shared, so always check q_info->ref_cnt
 */
static int dp_alloc_rx_ippu_q(int inst, int portid, int subif_ix)
{
	struct dp_ring_pktdeq *dp_port_deq_rinfo;
	struct cqm_deq_ring_info *deq_rinfo;
	struct hal_priv *priv = HAL(inst);
	struct ppv4_q_sch_port *q_port;
	struct dp_port_info *port_info;
	struct dp_subif_info *sif;
	struct q_info *q_info;
	int ret = DP_FAILURE;
	int i;

	q_port = dp_kzalloc(sizeof(*q_port), GFP_ATOMIC);
	if (!q_port)
		return DP_FAILURE;

	port_info = get_dp_port_info(inst, portid);
	sif = get_dp_port_subif(port_info, subif_ix);

	for (i = 0; i < port_info->num_rx_ippu; i++) {
		dp_port_deq_rinfo = port_info->rx_ippu[i];
		q_port->cqe_deq = dp_port_deq_rinfo->attr.ring.index;

		_DP_DEBUG(DP_DBG_FLAG_QOS, "DPM: %s: %s=%d %s=0x%px %s=0x%px %s=0x%px %s=%d\n",
			 __func__,
			 "cqe_rxippu", q_port->cqe_deq,
			 "priv", priv,
			 "deq_ring_stat", priv ? priv->deq_ring_stat : NULL,
			 "qdev", priv ? priv->qdev : NULL,
			 "inst", inst);
		deq_rinfo = get_dp_deqring_info(inst, q_port->cqe_deq);
		if (deq_rinfo->ref_cnt) {
			/*Queue for this IPPU ring is already allocated*/
			q_port->qid = deq_rinfo->qid;
			q_info = get_dp_q_info(inst, q_port->qid);
			q_port->q_node = q_info->q_node_id;
			if(!q_info->ref_cnt)
				pr_err("DPM: %s, something wrong, rx ippu deq->refcnt: %d and q->refcnt: %d not aligned\n",
						__func__, deq_rinfo->ref_cnt, q_info->ref_cnt);
			q_info->ref_cnt++;
			q_port->port_node = priv->deq_ring_stat[q_port->cqe_deq].node_id;
		} else {
			q_port->tx_pkt_credit = deq_rinfo->deq.pkt_credit;
			q_port->tx_ring_addr = deq_rinfo->deq.txpush_addr_qos;
			q_port->tx_ring_addr_push = deq_rinfo->deq.attr.paddr;
			q_port->tx_ring_size = deq_rinfo->deq.attr.size;
			q_port->inst = inst;
			q_port->dp_port = portid;
			q_port->vap = subif_ix;
			if (alloc_q(inst, q_port, deq_rinfo))
				goto free;
		}
		deq_rinfo->ref_cnt++;

		/* update ippu qid into subif table */
		sif->rx_ippu_qid_list[i] = q_port->qid;
		sif->rx_ippu_q_node[i] = q_port->q_node;
		sif->rx_ippu_qos_deq_port[i] = q_port->port_node;
		sif->rx_ippu_ring[i] = q_port->cqe_deq;
		sif->num_rx_ippu++;
	}
	ret = DP_SUCCESS;
free:
	kfree(q_port);
	return ret;
}

static int dp_alloc_deq_q(int inst, int portid, int subif_ix, struct subif_platform_data *data)
{
	struct dp_ring_pktdeq *dp_port_deq_rinfo;
	struct cqm_deq_ring_info *deq_rinfo;
	struct dp_gpid_tx_info *gpid_info;
	struct hal_priv *priv = HAL(inst);
	struct ppv4_q_sch_port *q_port;
	struct dp_port_info *port_info;
	int deq_ring_idx = 0, i = 0;
	struct dp_subif_info *sif;
	struct q_info *q_info;
	int ret = DP_FAILURE;

	if (!data || !data->subif_data) {
		pr_err("DPM: %s: data NULL or subif_data NULL\n", __func__);
		return DP_FAILURE;
	}

	if (!priv) {
		pr_err("DPM: %s: priv NULL\n", __func__);
		return DP_FAILURE;
	}

	if (data->subif_data->flag_ops & DP_SUBIF_DEQRING_NUM &&
	    data->subif_data->num_deq_ring > DP_MAX_DEQ_PER_DEV) {
		//TODO: VBOLLA: instead above macro
		//can i use this macro: CQM_MAX_NUM_DEQ_RING_PER_ENABLE
		pr_err("DPM: %s: %s(%d), cannot be more than max Q per subif %d\n",
		       __func__, "deq_ring", data->subif_data->num_deq_ring,
		       DP_MAX_DEQ_PER_DEV);
		return DP_FAILURE;
	}

	if (data->subif_data)
		deq_ring_idx = data->subif_data->deq_ring_idx;

	port_info = get_dp_port_info(inst, portid);
	sif = get_dp_port_subif(port_info, subif_ix);

	if (deq_ring_idx >= port_info->num_deq) {
		pr_err("DPM: %s: Wrong deq_ring_idx(%d), should < %d\n",
		       __func__, deq_ring_idx, port_info->num_deq);
		return DP_FAILURE;
	}

	gpid_info = &data->subif_data->gpid_tx_info;

	if (gpid_info->f_min_pkt_len)
		sif->min_pkt_len = gpid_info->min_pkt_len;
	else
		sif->min_pkt_len = 0;

	sif->min_pkt_len_cfg = get_min_pkt_len_cfg(sif->min_pkt_len);
	if (sif->min_pkt_len_cfg >= PP_NUM_TX_MIN_PKT_LENS) {
		pr_err("DPM: %s: unsupported min_pkt_len = %u\n",
		       __func__, sif->min_pkt_len);
		return DP_FAILURE;
	}

	data->act = 0;

	q_port = dp_kzalloc(sizeof(*q_port), GFP_ATOMIC);
	if (!q_port)
		return DP_FAILURE;

	for (i = 0; i < data->subif_data->num_deq_ring; i++) {
		dp_port_deq_rinfo = port_info->deq[deq_ring_idx + i];
		q_port->cqe_deq = dp_port_deq_rinfo->attr.ring.index;

		_DP_DEBUG(DP_DBG_FLAG_QOS, "DPM: %s: %s=%d %s=0x%px %s=0x%px %s=0x%px %s=%d\n",
			 __func__,
			 "cqe_deq", q_port->cqe_deq,
			 "priv", priv,
			 "deq_ring_stat", priv ? priv->deq_ring_stat : NULL,
			 "qdev", priv ? priv->qdev : NULL,
			 "inst", inst);
		deq_rinfo = get_dp_deqring_info(inst, q_port->cqe_deq);
		q_port->tx_pkt_credit = deq_rinfo->deq.pkt_credit;
		q_port->tx_ring_addr = deq_rinfo->deq.txpush_addr_qos;
		q_port->tx_ring_addr_push = deq_rinfo->deq.attr.paddr;
		q_port->tx_ring_size = deq_rinfo->deq.attr.size;
		q_port->inst = inst;
		q_port->dp_port = portid;
		q_port->vap = subif_ix;

		if (!(data->subif_data->flag_ops &
		      (DP_SUBIF_SPECIFIC_Q | DP_SUBIF_AUTO_NEW_Q))) {
			if (deq_rinfo->qid == -1)
				data->subif_data->flag_ops |= DP_SUBIF_AUTO_NEW_Q;
		}

		if (data->subif_data->flag_ops & DP_SUBIF_AUTO_NEW_Q) {
			if (alloc_q(inst, q_port, deq_rinfo))
				goto free;

		} else if (data->subif_data->flag_ops & DP_SUBIF_SPECIFIC_Q) {
			DP_DEBUG(DP_DBG_FLAG_QOS,
				 "Queue decision: %s\n", "specified_queue");

			/* Single GEM or 1 Subif have only 1 Qid
			 * Multiple GEM or Multiple Subif can have same Qid
			 */
			q_info = get_dp_q_info(inst, data->subif_data->q_id);

			if (!q_info->flag) {
				q_info->need_free = 0; /* caller Q,so no free */
				q_info->cqm_dequeue_ring = q_port->cqe_deq;
			} else {
				/* Multiple subif can have same Q, we increment
				 * Q reference count if Queue created by DPM
				 */
				q_info->ref_cnt++;
			}

			q_port->qid = data->subif_data->q_id;
			q_port->q_node = q_info->q_node_id;

		} else {
			DP_DEBUG(DP_DBG_FLAG_QOS,
				 "Queue decision:%s\n", "shared_queue");

			/* auto sharing queue: if go to here,
			 * it means sharing queue
			 * is ready and it is created by previous
			 * dp_register_subif_ext
			 */

			/* get already stored q_node_id/qos_port id to q_port */
			q_port->qid = deq_rinfo->qid;
			q_info = get_dp_q_info(inst, q_port->qid);
			q_port->q_node = q_info->q_node_id;
			get_dp_q_info(inst, q_port->qid)->ref_cnt++;
			q_port->port_node = priv->deq_ring_stat[q_port->cqe_deq].node_id;
		}

		deq_rinfo->ref_cnt++;

		_DP_DEBUG(DP_DBG_FLAG_REG,
			 "DPM: %s: %s=%d %s=%d q[%d].cnt=%d cqm_ring[%d].cnt=%d\n",
			 __func__,
			 "dp_port", portid,
			 "vap", subif_ix,
			 q_port->qid, get_dp_q_info(inst, q_port->qid)->ref_cnt,
			 q_port->cqe_deq, deq_rinfo->ref_cnt);

		/* first CTP */
		if (deq_rinfo->ref_cnt == 1)
			data->act = TRIGGER_CQE_DP_ENABLE;

		/* update caller dp_subif_data.q_id with allocated queue num */
		data->subif_data->q_id = q_port->qid;
		/* update subif table */
		sif->def_qid_list[i] = q_port->qid;
		sif->q_node[i] = q_port->q_node;
		sif->qos_deq_port[i] = q_port->port_node;
		sif->cqm_deq_ring[i] = q_port->cqe_deq;
		sif->num_deq_ring++;
		sif->deq_ring_idx = deq_ring_idx;
		//TODO: VBOLLA: port_info->subif_info[subif_ix].cqm_ring_idx = deq_ring_idx; seems already doing
		//above
	}
	ret = DP_SUCCESS;
free:
	kfree(q_port);
	return ret;
}

/*skew can be a positive or negative value*/
static void aca_ref_count_handler(int inst, int port_id, int skew)
{
	struct dp_port_info *port_info;
	struct cqm_enq_ring_info *ering;
	struct cqm_req_ring_info *rqring;
	struct cqm_ret_ring_info *rtring;
	int i;

	port_info = get_dp_port_info(inst, port_id);

	/*Check for DP_F_ACA flag*/
	if (!(port_info->alloc_flags & DP_F_ACA))
		return;

	for (i = 0; i < port_info->num_enq; i++) {
		ering = get_dp_enqring_info(inst, port_info->enq[i]->attr.ring.index);
		ering->ref_cnt = ering->ref_cnt + (skew);
	}

	for (i = 0; i < port_info->num_req; i++) {
		rqring = get_dp_reqring_info(inst, port_info->req[i]->attr.ring.index);
		rqring->ref_cnt = rqring->ref_cnt + (skew);
	}

	for (i = 0; i < port_info->num_ret; i++) {
		rtring = get_dp_retring_info(inst, port_info->ret[i]->attr.ring.index);
		rtring->ref_cnt = rtring->ref_cnt + (skew);
	}
}

static int subif_hw_set(int inst, int portid, int subif_ix,
			struct subif_platform_data *data, u32 flags)
{
	if (!data || !data->subif_data) {
		pr_err("DPM: %s: data NULL or subif_data NULL\n", __func__);
		return DP_FAILURE;
	}

	if (dp_alloc_tx_ippu_q(inst, portid, subif_ix)) {
		pr_err("DPM: %s, tx_ippu_q alloc failed\n", __func__);
		return DP_FAILURE;
	}
	if (dp_alloc_rx_ippu_q(inst, portid, subif_ix)) {
		pr_err("DPM: %s, tx_ippu_q alloc failed\n", __func__);
		return DP_FAILURE;
	}
	if (dp_alloc_deq_q(inst, portid, subif_ix, data)) {
		pr_err("DPM: %s, tx_ippu_q alloc failed\n", __func__);
		return DP_FAILURE;
	}

	/*Increase enq/ret/req ring ref count if any for debuging purpose*/
	aca_ref_count_handler(inst, portid, +1);

	if (dp_subif_pp_set(inst, portid, subif_ix,
			    data->subif_data->flag_ops)) {
		pr_err("DPM: %s: dp_subif_pp_set fail for dpid/vap=%d/%d\n",
		       __func__, portid, subif_ix);
		return DP_FAILURE;
	}

	/* copy port level adp_templ dw3 to subif level, and update
	 * gpid port as gpid assignment happens at subif
	 */
	init_adp_template(portid, subif_ix, true);

	return DP_SUCCESS;
}

static int subif_hw_reset(int inst, int portid, int subif_ix,
			  struct subif_platform_data *data, u32 flags)
{
	int qid, idx, q_num;
	int cqm_deq_ring;
	struct dp_port_info *port_info = get_dp_port_info(inst, portid);
	struct dp_subif_info *sif = get_dp_port_subif(port_info, subif_ix);
	struct cqm_deq_ring_info *deq_rinfo;
	struct q_info *q_info;

	if (dp_subif_pp_reset(inst, portid, subif_ix,
			      sif->data_flag_ops)) {
		pr_err("DPM: %s: dp_subif_pp_reset fail: portid=%d vap=%d\n",
		       __func__, portid, subif_ix);
		return DP_FAILURE;
	}

	/*Decrease enq/ret/req ring ref count if any for debuging purpose*/
	aca_ref_count_handler(inst, portid, -1);

	q_num = sif->num_qid;
	for (idx = 0; idx < q_num; idx++) {
		qid = sif->def_qid_list[idx];
		cqm_deq_ring = sif->cqm_deq_ring[idx];
		deq_rinfo = get_dp_deqring_info(inst, cqm_deq_ring);
		q_info = get_dp_q_info(inst, qid);

		/* santity check table */
		if (!deq_rinfo->ref_cnt) {
			pr_err("DPM: %s: Why dp_deq_ring_tbl[%d][%d].ref_cnt Zero\n",
			       __func__, inst, cqm_deq_ring);
			return DP_FAILURE;
		}
		/* update queue/port/sched/bp_pmapper table's ref_cnt */
		deq_rinfo->ref_cnt--;

		cqm_deq_ring = q_info->cqm_dequeue_ring;
		deq_rinfo = get_dp_deqring_info(inst, cqm_deq_ring);
		free_q(inst, portid, qid, deq_rinfo, cqm_deq_ring);
		sif->num_qid--;

		DP_DEBUG(DP_DBG_FLAG_REG,
			 "%s:%s=%d %s=%d q[%d].cnt=%d cqm_ring[%d].cnt=%d\n",
			 __func__,
			 "dp_port", portid,
			 "vap", subif_ix,
			 qid, q_info->ref_cnt,
			 cqm_deq_ring, deq_rinfo->ref_cnt);

		if (!port_info->num_subif && deq_rinfo->ref_cnt) {
			pr_err("DPM: %s: num_subif(%d) not match %s[%d][%d].ref_cnt\n",
			       __func__,
			       port_info->num_subif,
			       "dp_deq_ring_tbl", inst, cqm_deq_ring);
			return DP_FAILURE;
		}
	}

	/*TX ippu queue clean*/
	q_num = sif->num_tx_ippu;
	for (idx = 0; idx < q_num; idx++) {
		qid = sif->tx_ippu_qid_list[idx];
		cqm_deq_ring = sif->tx_ippu_ring[idx];
		deq_rinfo = get_dp_deqring_info(inst, cqm_deq_ring);
		q_info = get_dp_q_info(inst, qid);

		/* santity check table */
		if (!deq_rinfo->ref_cnt) {
			pr_err("DPM: %s: Why dp_deq_ring_tbl[%d][%d].ref_cnt Zero\n",
			       __func__, inst, cqm_deq_ring);
			return DP_FAILURE;
		}
		/* update queue/port/sched/bp_pmapper table's ref_cnt */
		deq_rinfo->ref_cnt--;

		cqm_deq_ring = q_info->cqm_dequeue_ring;
		deq_rinfo = get_dp_deqring_info(inst, cqm_deq_ring);

		free_q(inst, portid, qid, deq_rinfo, cqm_deq_ring);
		sif->num_tx_ippu--;

		DP_DEBUG(DP_DBG_FLAG_REG,
			 "%s:%s=%d %s=%d q[%d].cnt=%d cqm_ring[%d].cnt=%d\n",
			 __func__,
			 "dp_port", portid,
			 "vap", subif_ix,
			 qid, q_info->ref_cnt,
			 cqm_deq_ring, deq_rinfo->ref_cnt);

		if (!port_info->num_subif && deq_rinfo->ref_cnt) {
			pr_err("DPM: %s: num_subif(%d) not match %s[%d][%d].ref_cnt\n",
			       __func__,
			       port_info->num_subif,
			       "dp_deq_ring_tbl", inst, cqm_deq_ring);
			return DP_FAILURE;
		}
	}

	/*RX ippu queue clean*/
	q_num = sif->num_rx_ippu;
	for (idx = 0; idx < q_num; idx++) {
		qid = sif->rx_ippu_qid_list[idx];
		cqm_deq_ring = sif->rx_ippu_ring[idx];
		deq_rinfo = get_dp_deqring_info(inst, cqm_deq_ring);
		q_info = get_dp_q_info(inst, qid);

		/* santity check table */
		if (!deq_rinfo->ref_cnt) {
			pr_err("DPM: %s: Why dp_deq_ring_tbl[%d][%d].ref_cnt Zero\n",
			       __func__, inst, cqm_deq_ring);
			return DP_FAILURE;
		}
		/* update queue/port/sched/bp_pmapper table's ref_cnt */
		deq_rinfo->ref_cnt--;

		cqm_deq_ring = q_info->cqm_dequeue_ring;
		deq_rinfo = get_dp_deqring_info(inst, cqm_deq_ring);

		free_q(inst, portid, qid, deq_rinfo, cqm_deq_ring);
		sif->num_rx_ippu--;

		DP_DEBUG(DP_DBG_FLAG_REG,
			 "%s:%s=%d %s=%d q[%d].cnt=%d cqm_ring[%d].cnt=%d\n",
			 __func__,
			 "dp_port", portid,
			 "vap", subif_ix,
			 qid, q_info->ref_cnt,
			 cqm_deq_ring, deq_rinfo->ref_cnt);

		if (!port_info->num_subif && deq_rinfo->ref_cnt) {
			pr_err("DPM: %s: num_subif(%d) not match %s[%d][%d].ref_cnt\n",
			       __func__,
			       port_info->num_subif,
			       "dp_deq_ring_tbl", inst, cqm_deq_ring);
			return DP_FAILURE;
		}
	}
	return DP_SUCCESS;
}

/*Set basic BP/CTP */
static int subif_platform_set(int inst, int portid, int subif_ix,
			      struct subif_platform_data *data, u32 flags)
{
	if (flags & DP_F_DEREGISTER)
		return subif_hw_reset(inst, portid, subif_ix, data, flags);
	return subif_hw_set(inst, portid, subif_ix, data, flags);
}

static int dp_ctp_tc_map_set(struct dp_tc_cfg *tc, int flag,
			     struct dp_meter_subif *mtr_subif)
{
#if 0 //GSW_ENABLE
	struct core_ops *gsw_ops = dp_port_prop[mtr_subif->inst].ops[0];
	struct ctp_ops *gsw_ctp;
	GSW_CTP_portConfig_t *ctp_tc_cfg;

	if (!mtr_subif) {
		pr_err("DPM: %s: mtr_subif struct NULL\n", __func__);
		return DP_FAILURE;
	}

	if (mtr_subif->subif.flag_pmapper) {
		pr_err("DPM: %s: Cannot support ctp tc set for pmmapper dev(%s)\n",
		       __func__,
		       tc->dev ? tc->dev->name : "NULL");
		return DP_FAILURE;
	}

	ctp_tc_cfg = dp_kzalloc(sizeof(*ctp_tc_cfg), GFP_ATOMIC);
	if (!ctp_tc_cfg)
		return DP_FAILURE;
	gsw_ctp = &gsw_ops->gsw_ctp_ops;
	ctp_tc_cfg->nLogicalPortId = mtr_subif->subif.port_id;
	ctp_tc_cfg->nSubIfIdGroup = mtr_subif->subif.subif;

	if (gsw_ctp->CTP_PortConfigGet(gsw_ops, ctp_tc_cfg)) {
		pr_err("DPM: %s: Failed to get CTP info for %s=%d %s=%d\n",
		       __func__,
		       "ep", mtr_subif->subif.port_id,
		       "subif", mtr_subif->subif.subif);
		kfree(ctp_tc_cfg);
		return DP_FAILURE;
	}

	ctp_tc_cfg->eMask = GSW_CTP_PORT_CONFIG_MASK_FORCE_TRAFFIC_CLASS;
	ctp_tc_cfg->nDefaultTrafficClass = tc->tc;
	if (tc->force)
		ctp_tc_cfg->bForcedTrafficClass = tc->force;
	else
		ctp_tc_cfg->bForcedTrafficClass = 0;

	if (gsw_ctp->CTP_PortConfigSet(gsw_ops, ctp_tc_cfg)) {
		pr_err("DPM: %s: CTP tc set fail for %s=%d %s=%d %s=%d %s=%d\n",
		       __func__,
		       "ep", mtr_subif->subif.port_id,
		       "subif", mtr_subif->subif.subif,
		       "tc", tc->tc,
		       "force", tc->force);
		kfree(ctp_tc_cfg);
		return DP_FAILURE;
	}
	kfree(ctp_tc_cfg);
#endif
	return DP_SUCCESS;
}

static int not_valid_rx_ep(int ep)
{
	return (((ep >= 3) && (ep <= 6)) || (ep == 2) || (ep > 15));
}


static void get_adp_templ(int index,
			       struct adp_tx_desc_1 *desc_1,
			       struct adp_tx_desc_2 *desc_2,
			       struct adp_tx_desc_3 *desc_3,
			       struct dp_port_info *dp_info,
			       struct dp_subif_info *sif)
{
	struct adp_tx_desc_1 *adp1_mask =
		(struct adp_tx_desc_1 *)&dp_info->desc_dw_mask[index][1];
	struct adp_tx_desc_2 *adp2_mask =
		(struct adp_tx_desc_2 *)&dp_info->desc_dw_mask[index][2];
	struct adp_tx_desc_3 *adp3_mask =
		(struct adp_tx_desc_3 *)&dp_info->desc_dw_mask[index][3];

	struct adp_tx_desc_1 *adp1_templ =
		(struct adp_tx_desc_1 *)&dp_info->desc_dw_templ[index][1];
	struct adp_tx_desc_2 *adp2_templ =
		(struct adp_tx_desc_2 *)&dp_info->desc_dw_templ[index][2];
	/*Note: we get dw3 from subif as gpid is decided at subif level*/
	struct adp_tx_desc_3 *adp3_templ =
		(struct adp_tx_desc_3 *)&sif->dw3[index];

	desc_1->all = (desc_1->all & adp1_mask->all) | adp1_templ->all;
	desc_2->all = (desc_2->all & adp2_mask->all) | adp2_templ->all;
	desc_3->all = (desc_3->all & adp3_mask->all) | adp3_templ->all;
}

static int check_csum_cap(void)
{
	return DP_SUCCESS;
}

int register_dp_capability(int flag)
{
	struct dp_hw_cap *cap;

	cap = dp_kzalloc(sizeof(*cap), GFP_ATOMIC);
	if (!cap)
		return DP_FAILURE;
	cap->info.dp_platform_set = dp_platform_set;
	cap->info.port_platform_set = port_platform_set;
	cap->info.dev_platform_set = dev_platform_set;
	cap->info.init_adp_template = init_adp_template;
	cap->info.subif_platform_set = subif_platform_set;
	cap->info.subif_platform_change_mtu = subif_platform_change_mtu;
	cap->info.not_valid_rx_ep = not_valid_rx_ep;
	cap->info.check_csum_cap = check_csum_cap;
	cap->info.get_adp_templ = get_adp_templ;
	cap->info.dump_rx_adp_desc = dump_rx_adp_desc;
	cap->info.dump_tx_adp_desc = dump_tx_adp_desc;
	cap->info.dp_qos_platform_set = qos_platform_set;
	cap->info.dp_ctp_tc_map_set = dp_ctp_tc_map_set;
	cap->info.dp_get_queue_mib = dp_get_queue_mib;

	cap->info.cap.hw_tso = 0;
	cap->info.cap.hw_gso = 0;
	dp_strlcpy(cap->info.cap.qos_eng_name, "ppv4",
		   sizeof(cap->info.cap.qos_eng_name));
	dp_strlcpy(cap->info.cap.pkt_eng_name, "mpe",
		   sizeof(cap->info.cap.pkt_eng_name));
	cap->info.cap.max_num_queues = DP_MAX_QUEUE_NUM;
	cap->info.cap.max_num_scheds = DP_MAX_NODES;
	cap->info.cap.max_num_qos_ports = DP_MAX_PPV4_PORT;
	cap->info.cap.max_num_deq_rings = DP_MAX_CQM_DEQ;
	cap->info.cap.max_num_subif_per_port = MAX_SUBIF_PER_PORT;
	cap->info.cap.max_cpu = CQM_MAX_CPU;
	//cap->info.dp_tc_vlan_set = tc_vlan_set;
	cap->info.dp_rx = _dp_rx;
	cap->info.dp_tx = _dp_tx;

	cap->info.cap.max_port_per_cpu = DP_MAX_RING_PER_CPU;
	cap->info.cap.max_num_spl_conn = DP_MAX_SPL_CONN;
	cap->info.dp_spl_conn = _dp_spl_conn;
	cap->info.dp_spl_conn_get = _dp_spl_conn_get;
	cap->info.dp_set_io_port = dp_set_io_port;

	//if (dp_get_gswip_cap(cap, flag)) {
//		kfree(cap);
//		pr_err("DPM: %s: dp_get_gswip_cap fail\n", __func__);
//		return DP_FAILURE;
//	}

	cap->info.cap.max_num_dp_ports = 16;
	cap->info.cap.max_num_subif = 288;
//	cap->info.cap.max_num_bridge_port = 128;

	if (register_dp_hw_cap(cap, flag)) {
		kfree(cap);
		pr_err("DPM: %s: Why register_dp_hw_cap fail\n", __func__);
		return DP_FAILURE;
	}
	kfree(cap);
	return DP_SUCCESS;
}

void dp_dump_ud0(struct dp_pp_ud0 *ud0)
{
	pr_info("ud0 rx_port=%d l3/l4 offset=%d/%d session_id=%d\n",
		ud0->rx_port, ud0->int_l3_off, ud0->int_l4_off, ud0->sess_id);
}

void dp_dump_ud1(struct dp_pp_ud1 *ud1)
{
	pr_info("ud1 hashA/B/C=%x/%x/%x\n", ud1->hashA, ud1->hashB, ud1->hashC);
}


/* disable optimization in debug mode: pop */
DP_NO_OPTIMIZE_POP
