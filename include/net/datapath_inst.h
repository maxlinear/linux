// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020-2022 MaxLinear, Inc.
 * Copyright (C) 2020 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DATAPATH_INST_H
#define DATAPATH_INST_H

#include <net/switch_api/lantiq_gsw_api.h> /*Switch related structures */
#include <net/switch_api/lantiq_gsw_flow.h>
#include <net/switch_api/lantiq_gsw.h>
#include <net/switch_api/gsw_dev.h>

struct logic_dev;
struct subif_platform_data;
struct pmac_port_info;
struct pmac_port_info2;
struct gsw_itf;
struct dp_dev;

#define DP_MAX_GSW_HANDLE 2 /*! max GSW instance per SOC */
#define DP_MAX_MAC_HANDLE 11 /*! max MAC instance per SOC */

/*! enum for DP HW capability type */
enum DP_HW_CAP_TYPE {
	GSWIP30_TYPE = 0,
	GSWIP31_TYPE,
	GSWIP32_TYPE
};

/*! enum for DP HW version */
enum DP_HW_CAP_VER {
	GSWIP30_VER = 0,
	GSWIP31_VER,
	GSWIP32_VER
};

struct dp_meter_subif {
	int fid;
	int inst;
	dp_subif_t subif;
};

struct dp_inst_info {
	int inst; /*! for register, it will be filled by DP,
		   *  for de-register, the caller fill the instance id
		   */
	enum DP_HW_CAP_TYPE type;  /*! HW type */
	enum DP_HW_CAP_VER ver;  /*! HE version */
	struct core_ops *ops[DP_MAX_GSW_HANDLE]; /*! GSWIP ops handler*/
	struct mac_ops *mac_ops[DP_MAX_MAC_HANDLE]; /*! GSWIP ops handler*/
	int cbm_inst;  /*! CBM instance for this DP instance*/
	int qos_inst; /*! QOS instance for this DP instance*/
};

struct dp_tx_common;
struct dp_tc_vlan_info;
struct inst_info {
	enum DP_HW_CAP_TYPE type;
	enum DP_HW_CAP_VER ver;
	struct dp_cap cap;
	int swdev_flag;
	/* VLAN AWARE feature flag currently enabled only for PRX */
	int vlan_aware_flag;
	int (*dp_platform_set)(int inst, u32 flag);
	int (*port_platform_set)(int inst, u8 ep, struct dp_port_data *data,
				 uint32_t flags);
	int (*dev_platform_set)(int inst, u8 ep, struct dp_dev_data *data,
				uint32_t flags);
	int (*subif_platform_set_unexplicit)(int inst, int port_id,
					     struct logic_dev *dev,
					     u32 flag);
	int (*port_platform_set_unexplicit)(int inst, int port_id,
					    struct logic_dev *dev,
					    u32 flag);
	int (*subif_platform_set)(int inst, int portid, int subif_ix,
				  struct subif_platform_data *data,
				  u32 flags);
	int (*subif_platform_change_mtu)(int inst, int portid, int subif_ix,
					 u32 mtu);
	int (*proc_print_ctp_bp_info)(struct seq_file *s, int inst,
				      struct pmac_port_info *port,
				      int subif_index, u32 flag);
	void (*init_dma_pmac_template)(int portid, uint32_t flags);
	int (*not_valid_rx_ep)(int ep);
	void (*set_pmac_subif)(struct pmac_tx_hdr *pmac, int32_t subif);
	void (*update_port_vap)(int inst, u32 *ep, int *vap,
				struct sk_buff *skb,
				struct pmac_rx_hdr *pmac, char *decryp);
	int (*check_csum_cap)(void);
	void (*get_dma_pmac_templ)(int index, struct pmac_tx_hdr *pmac,
				   struct dma_tx_desc_0 *desc_0,
				   struct dma_tx_desc_1 *desc_1,
				   struct pmac_port_info *dp_info);
	void (*set_dma_pmac_reins_templ)(struct dma_tx_desc_0 *desc_0,
					 struct dma_tx_desc_1 *desc_1,
					 struct pmac_tx_hdr *pmac);
	int (*get_itf_start_end)(struct gsw_itf *itf_info, u16 *start,
				 u16 *end);
	void (*dump_rx_dma_desc)(struct dma_rx_desc_0 *desc_0,
				 struct dma_rx_desc_1 *desc_1,
				 struct dma_rx_desc_2 *desc_2,
				 struct dma_rx_desc_3 *desc_3);
	void (*dump_tx_dma_desc)(struct dma_tx_desc_0 *desc_0,
				 struct dma_tx_desc_1 *desc_1,
				 struct dma_tx_desc_2 *desc_2,
				 struct dma_tx_desc_3 *desc_3);
	void (*dump_rx_pmac)(struct pmac_rx_hdr *pmac);
	void (*dump_tx_pmac)(struct pmac_tx_hdr *pmac);
	int (*supported_logic_dev)(int inst, struct net_device *dev,
				   char *subif_name);
	int (*dp_pmac_set)(int inst, u32 port, dp_pmac_cfg_t *pmac_cfg);
	int (*dp_set_gsw_parser)(u8 flag, u8 cpu, u8 mpe1, u8 mpe2, u8 mpe3);
	int (*dp_get_gsw_parser)(u8 *cpu, u8 *mpe1, u8 *mpe2, u8 *mpe3);
	int (*dp_qos_platform_set)(int cmd_id, void *cfg, int flag);
	int (*dp_get_port_vap_mib)(dp_subif_t *subif_id, void *priv,
				   struct rtnl_link_stats64 *stats,
				   u32 flags);
	int (*dp_clear_netif_mib)(dp_subif_t *subif, void *priv, u32 flag);
	int (*dp_set_gsw_pmapper)(int inst, int bport, int lport,
				  struct dp_pmapper *mapper, u32 flag);
	int (*dp_get_gsw_pmapper)(int inst, int bport, int lport,
				  struct dp_pmapper *mapper, u32 flag);
	int (*dp_ctp_tc_map_set)(struct dp_tc_cfg *tc, int flag,
				 struct dp_meter_subif *mtr_subif);
	int (*dp_meter_alloc)(int inst, int *meterid, int flag);
	int (*dp_meter_add)(struct net_device *dev,
			    struct dp_meter_cfg *meter, int flag,
			    struct dp_meter_subif *mtr_subif);
	int (*dp_meter_del)(struct net_device *dev,
			    struct dp_meter_cfg *meter, int flag,
			    struct dp_meter_subif *mtr_subif);
	int (*dp_set_bp_attr)(struct dp_bp_attr *bp_attr, int bport, u32 flags);

	int32_t (*dp_rx)(struct sk_buff *skb, uint32_t flags);
	int32_t (*dp_tx)(struct sk_buff *skb, struct dp_tx_common *cmn);
	void (*dp_net_dev_get_ss_stat_strings)(struct net_device *dev,
					       u8 *data);
	int (*dp_net_dev_get_ss_stat_strings_count)(struct net_device *dev);
	void (*dp_net_dev_get_ethtool_stats)(struct net_device *dev,
					     struct ethtool_stats *stats,
					     u64 *data);
	int (*dp_spl_conn)(int inst, struct dp_spl_cfg *conn);
	int (*dp_spl_conn_get)(int inst, enum DP_SPL_TYPE type,
			       struct dp_spl_cfg *conns, u8 cnt);
	int (*dp_set_io_port)(int inst, int dpid, int vap, int type);
	int (*dp_alloc_bridge_port)(int inst, int port_id, int subif_ix,
				    int fid, int bp_member, int flags);
	int (*dp_free_bridge_port)(int inst, int bp);
	int (*dp_deq_update_info)(struct dp_subif_upd_info *info);
	int (*dp_set_ctp_bp)(int inst, int ctp, int portid, int bp,
			     struct subif_platform_data *data);
	int (*dp_get_queue_mib)(struct dp_qos_queue_info *info, int flag);
	int (*dp_cfg_domain_for_bp_member)(int inst, int bp);
	int (*swdev_alloc_bridge_id)(int inst);
	int (*swdev_free_brcfg)(int inst, u16 fid);
	int (*swdev_bridge_cfg_set)(int inst, u16 fid);
	int (*swdev_bridge_port_cfg_set)(struct br_info *br_item, int inst,
					 int bport);
	int (*swdev_bridge_port_cfg_reset)(struct br_info *br_item,
					   int inst, int bport);
	int (*swdev_bridge_port_flags_set)(struct br_info *br_info,
					   int inst, int bport,
					   unsigned long flags);
	int (*swdev_bridge_port_flags_get)(int inst, int bport,
					   unsigned long *flags);
	int (*swdev_port_learning_limit_set)(int inst, int bport,
					     int learning_limit,
					     struct dp_dev *dp_dev);
	int (*dp_mac_set)(int bport, int fid, int inst, u8 *addr);
	int (*dp_mac_reset)(int bport, int fid, int inst, u8 *addr);
	int (*dp_cfg_vlan)(int inst, int vap, int ep); /* later can remove it */
	int (*swdev_bridge_mcast_flood)(int inst, int br_id, bool activate);
	int (*dp_tc_vlan_set)(struct core_ops *ops, struct dp_tc_vlan *vlan,
			      struct dp_tc_vlan_info *info,
			      int flag);
	int (*dp_qos_get_q_logic)(int cmd_id, void *cfg, int flag);
	int (*dp_get_deq_port_flag)(int inst, int deq_port);
};

struct dp_inst {
	enum DP_HW_CAP_TYPE type;
	enum DP_HW_CAP_VER ver;
	struct inst_info info;
};

struct dp_hw_cap {
	u8 valid;
	struct inst_info info;
};

struct inst_property {
	u8 valid;
	struct inst_info info;
	/*driver should know which HW to configure, esp for PCIe case */
	struct core_ops *ops[DP_MAX_GSW_HANDLE];
	struct mac_ops *mac_ops[DP_MAX_MAC_HANDLE];
	int cbm_inst;
	int qos_inst;
	void *priv_hal; /*private data per HAL */
};

int register_dp_cap_gswip30(int flag);
int register_dp_capability(int flag);
int register_dp_hw_cap(struct dp_hw_cap *info, u32 flag);

/*! request a new DP instance based on its HW type/version */
int dp_request_inst(struct dp_inst_info *info, u32 flag);

#endif /* DATAPATH_INST_H */
