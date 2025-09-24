// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MaxLinear, Inc.
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

#ifndef DATAPATH_API_H
#define DATAPATH_API_H
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/soc/mxl/datapath_api_ppv4.h>
#include <linux/soc/mxl/datapath_api_tx.h>
#include <linux/soc/mxl/datapath_api_rx.h>
#include <linux/soc/mxl/datapath_api_qos.h>
#include <linux/soc/mxl/mxl_cqm_api.h>
#include <linux/soc/mxl/datapath_dp_cqm.h>
#include <linux/soc/mxl/ippu_api.h>
#if IS_ENABLED(CONFIG_DPM_DATAPATH_CPUFREQ)
#include <linux/cpufreq.h>
#endif /*CONFIG_DPM_DATAPATH_CPUFREQ*/
#include <uapi/linux/ethtool.h>

/*! @mainpage Datapath Manager API
 * @section Basic Datapath Registration API
 * @section Datapath QOS HAL
 *
 * @file datapath_api.h
 * @brief This file contains all the API for datapath manager on the GRX500 and
 *future system. This will actually be split into different header files,
 *but collected together for understanding here.
 */
/*! @defgroup Datapath_MGR Datapath Manager Basic API
 *@brief All API and defines exported by Datapath Manager
 */
/*! @{ */
/*! @defgroup Datapath_Driver_Defines Datapath Driver Defines
 *@brief Defines used in the Datapath Driver
 */
/*! @defgroup Datapath_Driver_Structures Datapath Driver Structures
 *@brief Datapath Configuration Structures
 */
/*! @defgroup Datapath_Driver_API Datapath Driver Manager API
 *@brief  Datapath Driver Manager API
 *@brief  Datapath Driver Manager API
 */
/*! @defgroup PPA_Accel_API PPA Acceleration Driver API
 *@brief PPA Acceleration Driver API used for learning and getting
 *the information necessary to accelerate a flow
 */
/*! @defgroup Datapath_API_QOS Datapath QOS Manager API
 *@brief  Datapath QOS Manager API
 */
/*! @} */
#define DP_INTERFACE_ID   1 /*!< @brief DP Interface ID:
			     *   If there is interface change between DP and
			     *   upper drivers, like DCDP/PON ethernet driver,
			     *   this macro will increase
			     */
#define DP_MAX_INST  1  /*!< @brief maximum DP instance to support. It can be
			 *  change as as needed
			 */
#define DP_MAX_ETH_ALEN 6  /*!< @brief MAC Header Size */
#define DP_MAX_PMAC_LEN     8  /*!< @brief Maximum PMAC Header Size */

#define DP_MAX_POOL_SUBIF 4     /*!< maximum number of pool per subif
				 */
/*!< maximum number of DC PktEnq/RxOut ring */
#define DP_SPLCONN_PKTENQ_RING 1

/*!< maximum number of DC BufReq/RxIn ring */
#define DP_SPLCONN_BUFREQ_RING 1

/*!< maximum number of DC PktDeq/TxIn ring */
#define DP_SPLCONN_PKTDEQ_RING 1

/*!< maximum number of DC BufRet/Txout ring */
#define DP_SPLCONN_BUFRET_RING 1

/*!< maximum intermediate CQM dequeue port to IPPU from DC */
#define DP_MAX_SPLCONN_INTERM_RING 1

/*!< maximum number of DC PktEnq/RxOut ring */
#define DP_NUM_DC_PKTENQ_RING 1

/*!< maximum number of DC BufReq/RxIn ring */
#define DP_NUM_DC_BUFREQ_RING 2

/*!< maximum number of DC PktDeq/TxIn ring */
#define DP_NUM_DC_PKTDEQ_RING 16

/*!< maximum number of DC BufRet/Txout ring */
#define DP_NUM_DC_BUFRET_RING 1

/*!< maximum intermediate CQM dequeue port to IPPU from DC */
#define DP_MAX_DC_INTERM      1

/*!< maximum number of NON-DC PktDeq ring. For PON, it is up to 64 */
#define DP_NUM_PKTDEQ_RING    64

/*!< maximum number of NON-DC BufRet ring. Mainly for PON/Directpath
 * Note: for PON case, it is said HW fixed and maybe this setting
 * is set via PON ether driver's DTS instead of DPM Interfaces
 */
#define DP_NUM_BUFRET_RING    2

/*!< maximum intermediate CQM dequeue port from CPU to Non-DC via IPPU */
#define DP_MAX_INTERM    2 //TODO: VBOLLA: changed from 1 to 2, as now for DC 2 rx_ippu queues needed

#define DP_TS_HDRLEN	10    /*!< Default Timestamp Header Length to strip */
#define DP_DFL_SESS_NUM 16    /*!< Maximum default egress session per subif */
#define DP_FCS_LEN	4     /*!< FCS size in bytes */
#define DP_CPU_LPID	0 /*!< Default CPU port LPID */
/*!< get vap or subif group */
#define GET_VAP(subif, bit_shift, mask) (((subif) >> (bit_shift)) & (mask))
/*!< set vap or subif group */
#define SET_VAP(vap, bit_shift, mask) ((((u32)vap) & (mask)) << (bit_shift))

#define UMT_REG_MSG_CNT  3 /*!< Number of UMT Count registers*/

/*! @brief struct dp_umt_irq_attr for interrupt mode only
 */
struct dp_umt_irq_attr {
	/*!< umt count physical register address. set by CQM driver.
	 *   Buffer Request Ring: Peripheral = READER; CQM = WRITER
	 *   Buffer Return Ring:  Peripheral = WRITER; CQM = READER
	 *   Packet Descriptor Dequeue: Peripheral = READ; CQM = WRITE
	 *   Packet Descriptor Enqueue: Peripheral = WRITE; CQM = READ
	 *   p_cnt_regs[0]: UMT_ACCUM physical register address
	 *   p_cnt_regs[1]: UMT_ADD physical register address
	 *   p_cnt_regs[2]: UMT_SUB physical register address
	 *
	 */
	dma_addr_t p_cnt_regs[UMT_REG_MSG_CNT];

	/*!< umt count virtual register address. set by CQM driver since
	 *   it is mapped by CQM driver. Normally it is used by linux drivers.
	 *   v_cnt_regs[0]: UMT_ACCUM virtual register address
	 *   v_cnt_regs[1]: UMT_ADD virtual register address
	 *   v_cnt_regs[2]: UMT_SUB virtual register address
	 */
	void *v_cnt_regs[UMT_REG_MSG_CNT];

	/*!< interrupt trigger threshold. Valid range 1-255 to trigger msg
	 *   transfer, set by top level driver.
	 */
	u8 threshold;

	/*!< umt poll callback which follow NAPI framework
	 * This callback is set by top level driver and called by CQM driver.
	 * when IRQ happen, CQM driver will do necessary preparation and call
	 * these kinds callback one by one in the tasklet context.
	 * Before calling the callback, CQM driver must provide ring_poll_data
	 * ie, 3rd parameter as defined.
	 * At the same time, top level driver can handle maximum packets/buffers
	 * up to maximum of budget as 2nd parameter defined. when finishing this
	 * callback, it must return the number of processed packets/buffers
	 */
	int (*umt_poll)(struct napi_struct *napi, int budget, void *ring_poll_data);

	/*!< irq handler's data pointer, set by top level driver */
	void *ring_poll_data;

	/* Do we need error handle callback since error irq is inside CQM driver
	 * if yes, does error code enough, for example, packet enqueue fail ????
	 * set by top level driver
	 */
	int (*err_cb)(void *ring_poll_data, u32 error_code);

};

/*! @brief struct dp_umt_msg_attr for UMT msg mode only, ie, non-interrupt ode
 */
struct dp_umt_msg_attr {
	/* need send this umt msg or not, ie, send umt ADD or SUB msg depends on
	 * the ring:
	 *   SUB: for buffer return  and pkt enqueue ring
	 *   Add: for buffer request and pkt dequeue ring
	 * set by top level driver
	 */
	u8 enable_send;

	/*!< flag to suppress zero umt message or not
	 * set by top level driver
	 */
	u8 supress_zero_msg;
};

/*! @brief struct dp_umt_attr
 */
struct dp_umt_attr {
	/*!< 0: Interrupt mode, 1-UMT msg mode, set by top level driver */
	u8 msg_mode;

	/*!< umt message dst/target address, set by top level driver */
	dma_addr_t dst;

	union {
		/*!< umt message attribute in non-interrupt mode */
		struct dp_umt_msg_attr msg_attr;

		/*!< interrupt mode umt attribute */
		struct dp_umt_irq_attr irq_attr;
	};
};

struct dp_ring_index {
	/*!< global CQM dequeue ring index/QOS port id
	 * valid if index >= 0
	 */
	int index;

	/*!< the cqm deqeueu port ID. For PON/PP NF case, it is not valid since
	 * those qos port not linked to CQM dequeue port at all.
	 */
	u32 port_id;
};

/*! @brief struct dp_rx_ring
 */
struct dp_ring_attr {
	/*!< [in/out] port/ring  */
	struct dp_ring_index ring;

	/*!< [in/out] ring size */
	int size;

	/*!< [in/out] ring physical address without any masking
	 * if this adress is used for PP QOS, it is required to do masking
	 * and please use another variable txpush_addr_qos in struct dp_ring_pktdeq
	 *
	 * Normally CQM driver will allocate this ring buffer for most use case
	 * But for PON or other device, they may prefer to allocate this buffer
	 * by themseleves. In this case, we made such assumption here:
	 * If caller provide non-zero paddr, it regards the parameter as input.
	 * otherwise it regards as output.
	 */
	void *paddr;

	/*!< [in/out] ring virtual address of paddr.
	 * For SW to simulate mem device, or host side drivers, like VPN driver
	 */
	void *vaddr;

	/*!< [in/out] umt attribute. Valid for memory port only */
	struct dp_umt_attr umt;
};

/*! @brief struct umt_common_attr
 */

struct dp_ring_pktdeq {
	/*!< pkt deq information */
	struct dp_ring_attr attr;

	/*!< QOS push addr after shift or mask from QOS HW point of view */
	void *txpush_addr_qos; // ??? Not sure whether still need this variable in Topaz

	/* dma ch id: valid only for CQM dequeue port to IPPU/EPPU
	 * 0xFFFFFFFF ((u32)-1) means not valid
	 */
	u32 dma_ch_id;

	/*!< [in/out] port tx packet credit */
	u32 pkt_credit;

	/* [in] port tx bytes credit */
	u32 b_credit;

	/*!< [out] number of policy, depend on pool_policy_valid */
	u8 num_policy;

	/*!< [out] policy base, depend on num_policy */
	int policy_base;

	/*!< [out] base policy_pool id, depend on num_policy */
	u16 pool_id;

	/*!< tx policy map */
	u8 policy_map;
};

struct dp_ring_pktenq {
	/*!< pkt enq information */
	struct dp_ring_attr attr;
};

#define DP_MAX_POLICY_PER_DEV  4  /*!< The maximum of polices supported */
struct dp_ring_bufreq {
	/*!< pkt req information */
	struct dp_ring_attr attr;

	/* pool/policy requirement */

	/*!< [out] number of policy for this rx ring */
	u8 num_policy;

	/*!< [in/out] minimal guarnateed number of buffer requirement for the policies */
	u32 min_guarantee[DP_MAX_POLICY_PER_DEV];

	/*!< [in/out] buffer size requirement for the policies
	 *   DP/CQM will adjust to most matched BM pool
	 */
	int buf_size[DP_MAX_POLICY_PER_DEV];

	/*!< base police */
	int policy_base;

	/*!< rx policy map */
	u8 policy_map;

	/*!< base policy_pool id for this rx ring */
	u16 pool_id;

	/*!< optonal: HW may have limitiation with minimal buffer burst requirement */
	u16 min_burst;
};

struct dp_ring_bufret {
	/*!< pkt deq information */
	struct dp_ring_attr attr;
};

/*! @addtogroup Datapath_Driver_Structures */
/*! @brief  DP Sub-interface Data structure
 *@param port_id  Datapath Port Id corresponds to PMAC Port Id
 *@param subif    Sub-interface Id info. In GRX500, this 15 bits,
 *		only 13 bits in PAE are handled [14, 11:0]
 *\note
 */
enum DP_API_STATUS {
	DP_FAILURE = -1,  /*!< failure */
	DP_SUCCESS = 0, /*!< succeed */
};

#define DP_F_ENUM_OR_STRING(name, value, short_name) {name = value} /*!< @brief
								     *  macro
								     *  for
								     *  enum
								     */

/*! @brief Enumerator DP_F_FLAG */
enum DP_F_FLAG {
	DP_F_DEV_START     = BIT(0), /*!< DP device start*/
	DP_F_CPU           = BIT(1), /*!< For CPU */
	DP_F_FAST_ETH_LAN  = BIT(2), /*!< For Ethernet LAN device */
	DP_F_FAST_ETH_WAN  = BIT(3), /*!< For Ethernet WAN device */
	DP_F_FAST_WLAN     = BIT(4), /*!< For DirectConnect 6-bit VAP WLAN device*/
	DP_F_FAST_DSL      = BIT(5), /*!< For DSL device */
	DP_F_DIRECT        = BIT(6), /*!< For PPA Directpath/LitePath*/
	DP_F_DIRECTLINK    = BIT(7), /*!< For DirectLink/QCA device */
	DP_F_FAST_WLAN_EXT = BIT(8), /*!< 7-bit VAP WLAN device */
	DP_F_GPON          = BIT(9), /*!< For GPON device */
	DP_F_GINT          = BIT(10), /*!< For GINT device */
	DP_F_DOCSIS        = BIT(11), /*!< for DOCSIS device support*/
	DP_F_DEV_END       = BIT(12), /*!< DP device end */

	DP_F_DEREGISTER    = BIT(13), /*!< For de-allocate port only */
	DP_F_SHARE_RES     = BIT(14), /*!< if Wave has multiple radio share same ACA */
	DP_F_ACA           = BIT(15), /*!< peripheral PCI device via ACA*/
	/*Note, once add a new entry here int the enum,
	 *need to add new item in below macro DP_F_FLAG_LIST
	 */
};

/*! @brief DP_F_FLAG_LIST Note:per bit one variable */
#define DP_F_FLAG_LIST  { \
	DP_F_ENUM_OR_STRING(DP_F_DEV_START,           "DEV_START"),	\
	DP_F_ENUM_OR_STRING(DP_F_CPU,                 "CPU"),		\
	DP_F_ENUM_OR_STRING(DP_F_FAST_ETH_LAN,        "ETH_LAN"), 	\
	DP_F_ENUM_OR_STRING(DP_F_FAST_ETH_WAN,        "ETH_WAN"),	\
	DP_F_ENUM_OR_STRING(DP_F_FAST_WLAN,           "FAST_WLAN"),	\
	DP_F_ENUM_OR_STRING(DP_F_FAST_DSL,            "DSL"),		\
	DP_F_ENUM_OR_STRING(DP_F_DIRECT,              "DirectPath"), 	\
	DP_F_ENUM_OR_STRING(DP_F_DIRECTLINK,          "DirectLink"),	\
	DP_F_ENUM_OR_STRING(DP_F_FAST_WLAN_EXT,       "EXT_WLAN"),	\
	DP_F_ENUM_OR_STRING(DP_F_GPON,                "GPON"),		\
	DP_F_ENUM_OR_STRING(DP_F_GINT,                "GINT"),		\
	DP_F_ENUM_OR_STRING(DP_F_DOCSIS,              "DOCSIS"),	\
	DP_F_ENUM_OR_STRING(DP_F_DEV_END,             "DEV_END"),	\
	DP_F_ENUM_OR_STRING(DP_F_DEREGISTER,          "De-Register"),	\
	DP_F_ENUM_OR_STRING(DP_F_SHARE_RES,           "SHARE_ACA"),	\
	DP_F_ENUM_OR_STRING(DP_F_ACA,                 "ACA"),		\
}

#define DP_F_PORT_TUNNEL_DECAP  DP_F_LOOPBACK /*!< @brief Just for
					       *  back-compatible since
					       *  CQM is using old macro
					       *  DP_F_PORT_TUNNEL_DECAP
					       */
#define DP_COC_REQ_DP	1 /*!< @brief COC request from Datapath itself */
#define DP_COC_REQ_ETHERNET	2 /*!< @brief COC request from ethernet */
#define DP_COC_REQ_VRX318	4 /*!< @brief COC request from vrx318 */

enum PMAC_TCP_TYPE {
	TCP_OVER_IPV4 = 0,
	UDP_OVER_IPV4,
	TCP_OVER_IPV6,
	UDP_OVER_IPV6,
	TCP_OVER_IPV6_IPV4,
	UDP_OVER_IPV6_IPV4,
	TCP_OVER_IPV4_IPV6,
	UDP_OVER_IPV4_IPV6
};

/*! @brief pmapper mode */
enum DP_PMAP_MODE {
	DP_PMAP_PCP = 1,  /*!< PCP Mapper:with omci unmark frame option 1
			   *    ie, derive pcp fields from default
			   */
	DP_PMAP_DSCP,     /*!< PCP Mapper with omci unmark frame option 0,
			   *    ie, derive pcp fields from dscp bits
			   */
	DP_PMAP_DSCP_ONLY, /*!< DSCP mapper only: PON not using it */
	DP_PMAP_MAX       /*!< Not valid */
};

/*! @brief back pressure mode */
enum DP_BPRESS {
	DP_BPRESS_NA = 0,  /*!< NA: Not Applicable: for back compatible
			    * purpose and for those Non ACA port
			    */
	DP_BPRESS_DIS, /*!< disable back pressure */
	DP_BPRESS_EN, /*!< enable back pressure */
};

#define DP_MAX_CPU 4 /*!< @brief Max number of CPUs */
#define DP_PMAP_PCP_NUM 8  /*!< @brief  Max pcp entries supported per pmapper*/
#define DP_PMAP_DSCP_NUM 64 /*!<@brief  Max dscp entries supported per pmapper*/

/*!< @brief  max number of SUBIF per dev.
 *   Note: for docsis data service, it use 32 subif/GPID in LGM.
 *   If not consider docsis, the worst case only need support 8 subif for pon
 *   pmapper case. Later we may can futher reduce it according to PON new design
*/
#define DP_MAX_SUBIF_PER_DEV  8

/*!<@brief the maximum number of dequeue ring per subif
 * Note : normally one subif only have 1 dequeue ring.
 * But for some special case, like pon, one device can use up to
 * 8 dequeue port/ring
 * a) G.INT may have 8 dequeue rings per subif based on class/priority.
 * b) Wave700 may have 4 dequeue rings: 1 dedicated ring per band and
 *                                      1 shared by all bands
 * c) docsis: may up to 32
  */
#define DP_MAX_DEQ_PER_DEV 32
#define DP_MAX_DEQ_PER_SUBIF DP_MAX_DEQ_PER_DEV /* for back-comptible */
#define DP_PMAPPER_DISCARD_CTP 0xFFFF  /*!<@brief Discard ctp flag for pmapper*/

/**
 * @define number of maximum session group counters assigned to
 *  a session
 */
#define DP_SI_SGC_MAX              (8)
#define DP_SGC_INVALID             (U16_MAX)

/**
 * @define number of maximum traffic bucket meters assigned to a
 *  session
 */
#define DP_SI_TBM_MAX              (5)
#define DP_TBM_INVALID             (U16_MAX)

struct dp_egress {
	u16 cpu_gpid; /*!< gpid value */
	int q_id;  /*!< physical queue id */;
};

/* For exception sessions, only 2b reduced TC is used as in TPZ.
 * Maximum number of  Hostif per CPU
 */
#define DP_MAX_HOSTIF		4
/**
 * @struct dp_hif_datapath
 * @brief DP Host interface datapath information
 */
struct dp_hif_datapath {
	/*! up to 4 egress (logical queue and dp port), set unused */
	/*! queues to DP_QOS_INVALID_ID and unused ports to DP_PORT_INVALID */
	struct dp_egress eg[DP_MAX_CPU];

	/*! color to set on descriptor */
	u8  color;

	/*! sgc optional, sgc id's */
	u16 sgc[DP_SI_SGC_MAX];

	/*! tbm optional, tbm id's */
	u16 tbm[DP_SI_TBM_MAX];
};

/*! @brief structure for pmapper */
struct dp_pmapper {
	u32 pmapper_id;  /*!<pmapper_id : pmapper id*/
	enum DP_PMAP_MODE mode;  /*!< mode: pcp or dscp mapper*/
	u16 def_ctp;  /*!< default map: used for below cases:
		       *  pcp mode: for non-vlan packet case
		       *  dscp mode: for non-ip packet case
		       *  but according to PON OMCI requirement, in fact, it
		       *  should drop.
		       */
	u16 pcp_map[DP_PMAP_PCP_NUM];  /*!< For pcp mapper.  Should be
					*non-zero since CTP 0 reserved
					*/
	u16 dscp_map[DP_PMAP_DSCP_NUM]; /*!< For dscp mapper*/
};

/*! @brief structure for dp_subif_single
 *  Note: this is used for dp_register_subif and dp_xmit two API at present
 *        for one specific subif only
 */
typedef struct dp_subif_single {
	int inst;  /*!< dp instance id */
	int port_id; /*!< port_id: Datapath Port Id */
	int subif; /*!< subif: subif id. */

	/*!< Below variable is only used for dp_register_subif */
	u16 gpid; /*!< [output] the gpid is assigned */
} dp_subif_single_t;

/*! @brief structure for dp_subif
 *  Note: this is used for dp_get_netif_subifid API only at present/
 *  This API may return multiple subif under the specififed device.
 *  For example of PON pmapper deivce, docsis data device and so on.
 */
typedef struct dp_subif {
	int inst;  /*!< dp instance id */

	struct dp_ring_pktdeq deq[DP_MAX_DEQ_PER_DEV]; /*!< deq ring info*/
	struct dp_ring_pktdeq tx_ippu[DP_MAX_INTERM]; /*!< tx_ippu ring info*/
	struct dp_ring_pktdeq rx_ippu[DP_MAX_INTERM]; /*!< rx_ippu ring info*/
	int num_deq; /*!< Number of deq rings*/
	int num_tx_ippu; /*!< number of tx_ippu rings*/
	int num_rx_ippu; /*!< number of rx_ippu rings*/

	/*!< --------------------------------------------------------------
	 *!< start of placeholder for fast copy port_info.
	 *   Here all fileds must be same as defined in &struct dp_port_info
	 *  -----------------------------------------------------------------
	 */
	long port_cmn_start __attribute__((aligned(SMP_CACHE_BYTES)));
	int port_id;
	int alloc_flags; /*!< [out] the flag value is from the top level driver
			 *    during calling dp_alloc_port_ext
			 *   output for dp_get_netif_subifid
			 *   no use for dp_register_subif_ext
			 *   This is requested by PPA/DCDP to get original flag
			 *   the caller provided to DP during
			 *   dp_alloc_port
			 */
	u8  cqe_lu_mode;
	u32 gsw_mode;
	s16 gpid_spl;  /* special GPID:
			* alloc it at dp_alloc_port
			* config it at dp_register_dev for policy setting
			*/
	long port_cmn_end;
	/*!< --------------------------------------------------------------
	 *!< end  of placeholder for fast copy port_info.
	 *  -----------------------------------------------------------------
	 */

	/*!< --------------------------------------------------------------
	 *!< start of placeholder for fast copy subif_info.
	 *   Note, here all fileds must as defined as &struct dp_subif_info
	 *  -----------------------------------------------------------------
	 */
	long subif_cmn_start __attribute__((aligned(SMP_CACHE_BYTES)));
	union {
		u16 gpid; /*!< first gpid from the list*/
		/*!< only subif_num: valid number of GPID present in the array */
		u16 gpid_list[DP_MAX_SUBIF_PER_DEV];
	};
	u32 data_flag_ops; /* To store original flag from caller
			    * during dp_register_subif
			    * under data->flag_ops
			    */
	u8 num_qid; /*!< number of queue id*/
	union {
		u16 def_qid;    /* physical queue id Still keep it
			     * to be back-compatible for legacy
			     * platform and legacy integration
			     */
		/* physical queue id */
		u16 def_qid_list[DP_MAX_DEQ_PER_DEV];
	};
	union {
		u16 tx_ippu_qid;    /* physical queue id Still keep it
			     * to be back-compatible for legacy
			     * platform and legacy integration
			     */
		/* physical queue id */
		u16 tx_ippu_qid_list[DP_MAX_INTERM];
	};
	union {
		/*rx IPPU is only for ACA case*/
		u16 rx_ippu_qid;    /* physical queue id Still keep it
			     * to be back-compatible for legacy
			     * platform and legacy integration
			     */
		/* physical queue id */
		u16 rx_ippu_qid_list[DP_MAX_INTERM];
	};
	u32 dfl_eg_sess[DP_MAX_SUBIF_PER_DEV][DP_DFL_SESS_NUM];
					       /*!< [out]
						* default egress
						* session ids for
						* each gpid. This is
						* for CPU TX to DC
						* only
						*/
	u32 subif_groupid; /*!< [out] subif group id or vap */
	int subif_num; /*!< valid subif/ctp num.  ??? it is common_subif 
			*   output for dp_get_netif_subifid,
			*   no use for dp_register_subif_ext
			*/
	long subif_cmn_end;
	/*!< --------------------------------------------------------------
	 *!< end  of placeholder for fast copy subif_info.
	 *  -----------------------------------------------------------------
	 */

	union {
		s32 subif; /*!< [in/out] Sub-interface Id as HW defined
			    */
		s32 subif_list[DP_MAX_SUBIF_PER_DEV]; /*!< [in/out] subif list
						     *   Normally 1 subif per
						     *   dev. But for PON
						     *   pmapper case, multiple
						     *   subif per pmapper
						     *   device
						     */
	};
	int subif_flag[DP_MAX_SUBIF_PER_DEV]; /*!< the flag is used during
					     *   dp_register_subif_ext
					     *   output for dp_get_netif_subifid
					     *   no use for
					     *   dp_register_subif_ext.
					     *   This is requested by PPA/DCDP
					     *   to get original flag the caller
					     *   provided to DP during
					     *   dp_register_subif_ext
					     */
	u32 flag_pmapper : 1; /*!< [out] flag to indicate whether this
			       *   device is pmapper device.
			       *   For PON pmappper device: 1
			       *   For PON CTP device or other non PON device:0
			       *   valid for dp_get_netif_subifid only
			       */
	u32 peripheral_pvt; /*!< [out] peripheral private flag
			     * output for dp_get_netif_subifid
			     * no use for dp_register_subif_ext
			     * For example: DSL driver used to indicate Bonding
			     */
} dp_subif_t;

struct dp_port_prop {
	u32 vap_offset; /*shift bits to get vap value */
	u32 vap_mask; /*get final vap after bit shift */
	int alloc_flags; /* alloc flag */
	struct module *owner; /* port owner */
	u32 num_subif;  /* number of subif registered already */
	u16 max_subif;  /* maximum number of subif supported for this port */
	int status;  /* port status, registered or not */
	int port_id; /* dp_port id */
	u32 deq_ring_num; /* number of CQM dequeue port for this dp_port */
};

struct dp_subif_prop {
	int flags;
	struct net_device *netif;
	struct net_device *ctp_dev;
};

/*! @brief struct for dp_drv_mib */
typedef struct dp_drv_mib {
	u64 rx_drop_pkts;  /*!< rx drop pkts */
	u64 rx_error_pkts; /*!< rx error pkts */
	u64 tx_drop_pkts; /*!< tx drop pkts */
	u64 tx_error_pkts; /*!< tx error  pkts */
	u64 tx_pkts; /*!< tx pkts */
	u64 tx_bytes; /*!< tx bytes */
} dp_drv_mib_t;

/*! @brief struct for dp_buffer_info */
struct dp_buffer_info {
	int inst;         /*!< [in] dp instance id */
	phys_addr_t addr; /*!< [in] physical address of buffer to free */
	u32 policy_base;  /*!< [in] associated policy base*/
	int policy_num;   /*!< [in] associated policy number*/
};

/*! @brief struct for dp_buffer */
struct dp_dma_ch {
	u32 ch; /*!< DMA CH ID */
#define DP_DMA_IRQ_ENABLE  BIT(0)  /*!< enable DMA IRQ */
#define DP_DMA_IRQ_DISABLE BIT(1)  /*!< dsiable DMA IRQ */
	int flag; /*!< enable/disable flag */
};

/*! struct dp_aca_stop: dp stop ACA configuration */
struct dp_aca_stop {
	int inst; /*!< [in] DP instance ID */
};

typedef int32_t(*dp_rx_fn_t)(struct net_device *rxif, struct net_device *txif,
	struct sk_buff *skb, int32_t len);/*!< @brief   Device Receive
					   *   Function callback for packets
					   */
typedef int32_t(*dp_stop_tx_fn_t)(struct net_device *dev);/*!< @brief   The
							   * Driver Stop
							   *Tx function
							   *callback
							   */
typedef int32_t(*dp_restart_tx_fn_t)(struct net_device *dev); /*!< @brief Driver
							       * Restart Tx
							       * function
							       * callback
							       */
typedef int32_t(*dp_reset_mib_fn_t)(dp_subif_t *subif, int32_t flag);/*!< @brief
								      *Driver
								      *reset
								      *its mib
								      *counter
								      *callback
								      **/
typedef int32_t(*dp_get_mib_fn_t)(dp_subif_t *subif, dp_drv_mib_t *,
	int32_t flag); /*!< @brief   Driver get mib counter of the
			*specified subif interface.
			*/
typedef int32_t(*dp_get_netif_subifid_fn_t)(struct net_device *netif,
	struct sk_buff *skb, void *subif_data, uint8_t dst_mac[DP_MAX_ETH_ALEN],
	dp_subif_t *subif, uint32_t flags);	/*!< @brief   get subifid */
#if defined(CONFIG_DPM_DATAPATH_CPUFREQ)
typedef int32_t(*dp_coc_confirm_stat)(int new_state,
	int old_st, uint32_t f); /*!< @brief Confirm state
				  *   by COC
				  */
#endif
/*!
 *@brief Datapath Manager Registration Callback
 *@param rx_fn  Rx function callback
 *@param stop_fn    Stop Tx function callback for flow control
 * *@param restart_fn    Start Tx function callback for flow control
 *@param get_subifid_fn    Get Sub Interface Id of netif
 *@note
 */
typedef struct dp_cb {
	dp_rx_fn_t rx_fn;	/*!< Rx function callback */
	dp_stop_tx_fn_t stop_fn;/*!< Stop Tx function callback for
				 *flow control
				 */
	dp_restart_tx_fn_t restart_fn;	/*!< Start Tx function callback
					 *! For flow control
					 */
	dp_get_netif_subifid_fn_t get_subifid_fn; /*!< Get Sub Interface Id
						   *of netif/netdevice
						   */
	dp_reset_mib_fn_t reset_mib_fn;  /*!< reset registered device's network
					  *mib counters
					  */
	dp_get_mib_fn_t get_mib_fn; /*!< reset registered device's
				     *network mib counters
				     */
	irqreturn_t (*dma_rx_irq)(int irq, void *dev_instance); /*!< DMA RX IRQ
								 *   handler.
								 *   For 5G only
								 *   now.
								 */
	int (*aca_fw_stop)(struct dp_aca_stop *cfg, int flags); /*!< callback to
								 *   stop ACA FW
								 */
#if IS_ENABLED(CONFIG_DPM_DATAPATH_CPUFREQ)
	dp_coc_confirm_stat dp_coc_confirm_stat_fn; /*!< once COC confirm the
						     *state changed, Datatpath
						     *will notify Ethernet/
						     *VRX318 driver and
						     *Ethernet/VRX318 driver
						     *need to enable/disable
						     *interrupt or change
						     *threshold accordingly
						     */
#endif
} dp_cb_t;

/*!
 *@brief struct dp_subif_stats_t
 */
typedef struct dp_subif_stats_t {
	u64 rx_bytes; /*!< received bytes*/
	u64 rx_pkts; /*!< received packets*/
	u64 rx_disc_pkts; /*!< received discarded packets*/
	u64 rx_err_pkts; /*!< received errored packets*/
	u64 tx_bytes; /*!< transmitted bytes*/
	u64 tx_pkts; /*!< transmitted packets*/
	u64 tx_disc_pkts; /*!< transmitted discarded packets*/
	u64 tx_err_pkts; /*!< transmitted errored packets*/
} dp_subif_stats_t;

/*!
 *@brief Datapath Manager Port PMAC configuration structure
 *@param ig_pmac  Ingress PMAC configuration
 *@param eg_pmac  Egress PMAC configuration
 *@note GSW_PMAC_Ig_Cfg_t/GSW_PMAC_Eg_Cfg_t defined in GSWIP driver:
 *	<xway/switch-api/lantiq_gsw_api.h>
 */
typedef struct dp_pmac_cfg {
	u32 ig_pmac_flags;	/*!< one bit for one ingress_pmac_t fields */
	u32 eg_pmac_flags;	/*!< one bit for one egress_pmac_t fields */
#ifdef TOPAZ_CODE_ENABLE
	ingress_pmac_t ig_pmac;	/*!< Ingress PMAC configuration */
	egress_pmac_t eg_pmac;	/*!< Egress PMAC configuration */
#endif
} dp_pmac_cfg_t;

/*! @brief struct pon_subif_d */
struct pon_subif_d {
	s32 tcont_idx; /*!< relative tconf_idx map to CQE PON dequeuer port */
	s8 pcp;/*!< 0~7:valid pcp
		*   -1: non valid pcp value
		*/
};

/*! @brief enum DP_SUBIF_DATA_FLAG */
enum DP_SUBIF_DATA_FLAG {
	DP_SUBIF_AUTO_NEW_Q  = BIT(0), /*!< create new queue for this subif */
	DP_SUBIF_SPECIFIC_Q  = BIT(1), /*!< use the already configured queue as
					* specified by q_id in
					* struct dp_subif_data. This queue can
					* be created by caller itself, or
					* by last call of dp_register_subif_ext
					*/
	DP_SUBIF_DEQRING_NUM = BIT(2), /*!< Specify num of deq port per subif */
	DP_SUBIF_RX_FLAG     = BIT(3), /*!< To Specify if Rx enable/disable during
					* DP register subif using rx_en_flag
					* specified under struct dp_subif_data
					*/
	DP_SUBIF_CPU_QMAP    = BIT(4),
	DP_SUBIF_NO_HOSTIF   = BIT(4),
	DP_SUBIF_SEG_EN      = BIT(6), /*!< if this flag is set seg_en will be set to 1
					* to use FSQM buffer,
					*/
	DP_SUBIF_SEG_DIS     = BIT(7), /*!< if this flag is set seg_en=0 to use
					 * BM buffer
					 */
	DP_SUBIF_CPU         = BIT(8), /*!< Register special device as CPU port */
	DP_SUBIF_NO_US       = BIT(9), /*!< Register device for those subif which
					 * are not required to support upstream traffic,
					 * for example: docsis mpeg and DBG subif.
					 */
};

/*! @brief dp_subif_id struct for get_netif_subif */
struct dp_subif_cache {
	struct hlist_node hlist;
	dp_subif_t subif;
	struct net_device *dev;
	char name[IFNAMSIZ];
	dp_get_netif_subifid_fn_t subif_fn;  /*!< Get Sub Interface Id
					      * of netif/netdevice
					      */
	void *data;
	struct rcu_head rcu;
};

/*! @brief struct dp_subif_data */
struct dp_subif_data {
	int deq_ring_idx;  /*!< [in] range: relative index to deq ring/s allocated to
			   *  this device/dpid
			   *  For PON, it is tcont_idx,
			   *  For other device, normally its value is zero,
			   *    ie, from 1st deq_ring
			   */
	u16 num_deq_ring; /*!< [in] To specify number of deq_ring to be used */
	enum DP_SUBIF_DATA_FLAG flag_ops; /*!< flags */
	int q_id; /*!< [in,out]:
		   * [in]: valid only if DP_SUBIF_SPECIFIC_Q set in
		   *       \ref flag_ops
		   * [out]: queue alloted or reused for this subif
		   * Note: this queue can be created by caller,
		   *         or by dp_register_subif_ext itself in Pmapper case
		   */
	struct net_device *ctp_dev; /*!<  Optional CTP device.
				     * Mainly for PON CTP device under pmapper.
				     */
	dp_rx_fn_t rx_fn;      /*!< [in] subif RX callback */
	dp_get_netif_subifid_fn_t get_subifid_fn; /*! [in] get subif ID cb */
	u8 f_policy;   /*!< [in] flag to indicate whether need new
			*   policy for this subif or not.
			*   if this flat is set, it needs new tx/rx policy
			*   otherwise DP will use its existing base policy
			*   which is created during dp_register_dev_ext
			*/
	u16 tx_pkt_size;  /*!< [in] maximum packet size required
			   *   to alloc new policy for different cqm dequeue
			   *   port
			   *   valid only if f_tx_policy set
			   */
	/*!< [in] for GPID tx information: Valid only if \ref f_tx_policy set. */
	struct dp_gpid_tx_info gpid_tx_info;
	u16 tx_policy_base;   /*!< [out] txin_policy
			       *   if f_txin_policy set, this subif will need
			       *       create new policy
			       *   else DP will use its base policy which is
			       *       create during dp_register_dev_ext
			       */
	u8 tx_policy_num;	/*!< [out] */
	u16 rx_policy_base;	/*!< [out] */
	u8 rx_policy_num;	/*!< [out] */

	int txin_ring_size;  /*!< [in/out] ACA TXIN Ring size.
			      *   if input value is not zero, DP try to tune
			      *   down the pre-allocated TXIN ring buffer size.
			      *   Only allowed to tune down.
			      */
	void *txin_ring_phy_addr; /*!< [out] ACA TXIN Ring Buffer physical
				   *   address based on deq_port_idx
				   */
	void *credit_add_phy_addr; /*!< [out] PPv4 credit add physical address
				    *   which is valid only if flag
				    *   DP_F_NON_ACA_PORT is set during
				    *   dp_alloc_port_ext.
				    *   So far for 5G without using CQM DC port
				    */
	void *credit_left_phy_addr; /*!< [out] PPv4 credit left physical address
				     *   which is valid only if flag
				     *   DP_F_NON_ACA_PORT is set during
				     *   dp_alloc_port_ext
				     *   So far for 5G without using CQM DC port
				     */
	u32 rx_en_flag; /*!< [in] rx_en_flag = 1 - To enable dp_rx
			 * rx_en_flag = 0 - disable dp_rx
			 */
	u32 bm_policy_res_id; /*!< [in] buffer policy id */
};

/*! @brief enum DP_F_DATA_RESV_CQM_PORT */
enum dp_port_data_flag {
	DP_F_DATA_RESV_CQM_PORT = BIT(0), /*!< need reserve cqm multiple ports*/
	DP_F_DATA_ALLOC = BIT(1),
	DP_F_DATA_EVEN_FIRST = BIT(2), /*!< reserve dp_port in even number*/
	DP_F_DATA_RESV_Q = BIT(3), /*!< reserve QOS queue */
	DP_F_DATA_RESV_SCH = BIT(4), /*!< reserve QOS scheduler */
	DP_F_DATA_FCS_DISABLE = BIT(5), /*!< Disable FCS for PON port on SOC */
	DP_F_DATA_NO_CQM_DEQ = BIT(6), /*!< No mapped CQM dequeue port needed,
					*   instead DC device directly dequeue
					*   packet from PP QOS port via credit
					*   left and credit add
					*/
	DP_F_DATA_CONTINUOUS_Q_RESV = BIT(7), /*!< reserve continuous physical
					       *   queue ID
					       */
	DP_F_DATA_PON_HGU_SEC_LOOP = BIT(8), /*!< Once this flag is set,
					       * GSWIP second loop redirect to
					       * PMAC 1 will be configured
					       */
	DP_F_REM_FCS = BIT(9), /*!< Once this flag is set, dp_rx need to
				 *   remove 4 bytes of fcs in PRX
				 *   For LGM, we will use DMA descriptor Fcs
				 *   flag
				 */
};

/*! @brief struct dp_port_data_priv for DPM and CQM driver only */
struct dp_port_data_priv {
	/*udata: user data, for dp_alloc_port() */
	struct dp_port_data *udata;

	/* number of intermediate dequeue port ring:
	 *        CPU->CQM->IPPU->PP...-> device
	 * cpu ippu or TX ippu
	 */
	u8 num_ippu;

	/*!< pkt deq ring information or qos port information.
	 * It should be set according to the value of num_deq, up to
	 * DP_NUM_PKTDEQ_RING
	 *   For CQM non-memory port/ring, its content is set by CQM driver
	 *   For QOS port, its content is set by DPM
	 */
	struct dp_ring_pktdeq deq[DP_NUM_PKTDEQ_RING];

	/*!< tx_ippu ring*/
	struct dp_ring_pktdeq ippu[DP_MAX_INTERM];
};

/*! @brief struct dp_port_data */
struct dp_port_data {
	int flag_ops; /*!< [in] flag operation, refer to enum dp_port_data_flag
		       */
	u32 resv_num_port; /*!< valid only if DP_F_DATA_RESV_CQM_PORT is set.
			    * the number of cqm dequeue port to reserve.
			    * Currently mainly for Wave600 multiple radio but
			    * sharing same cqm dequeue port
			    */
	u32 start_port_no; /*!< valid only if DP_F_DATA_RESV_CQM_PORT is set */

	/* ---------------------------------------------------
	 * variables will be used by CQM driver: Begin
	 * note: 1) here only for non-DC related CQM/QOS port information
	 *       2) for directpath, 4 ring (pktenq/deq and bufreq/ret) are all
	 *          handled by CQM driver, not directpath driver
	 * ---------------------------------------------------
	 */

	/*!< [in/out] the number of PktDeq ring/QOS port for non-DC device
	 * For DC[out]: NA
	 * For Directpath/Ethernet streaming port[out]: set by CQM driver,
	 *                                              including priv->deq[]
	 * For PON port[in]: PON ether driver set it to the number as specified
	 *                   by OMCI MIB. Its value up to 64.
	 *                   priv->deq[] will be filled by DPM, not CQM driver.
	 */
	u8 num_deq;

	/*!< [in] this variable is only used by PON now for the QOS port's
	 * credit setting
	 */
	u8 credit;

	/*!< [out] this variable is only used by PON now for the base of QOS
	 * port ID
	 */
	u8 deq_ring_idx;

	/*!< [in] this variable is only used by PON now for the base of QOS
	 * TXMNGR_TXPORT_ADDRL setting (physical address), ie PON_DESC address
	 */
	void *deq_ring_base;

	/*!< [in] link speed capability, set by top level
	 *   network driver and used by CQM driver
	 */
	u32 link_speed_cap;

	/* ---------------------------------------------------
	 * variables will be used by CQM driver: End
	 * ---------------------------------------------------
	 */
};

/*! @brief struct dp_buf_type for free ACA/DC buffer */
struct dp_dc_buf {
	u16 f_policy_pool : 1; /*!< flag to indicate policy/pool valid or not */
	u16 policy; /*!< buffer policy, valid if f_policy_pool is set */
	u16 pool; /*!< buffer pool, valid if f_policy_pool is set.
		   *   For PRX300, it is a must to provide pool id or extract
		   *   from DP local table
		   */

	u16 num; /*!< number of buffers in the buffer list */
	void *buf; /*!< buffer list pointer */
};

/*! @brief enum DP_DEV_DATA_FLAG */
enum DP_DEV_DATA_FLAG {
	DP_F_DEV_RESV_Q = BIT(0), /*!< Reserve queues for this dev
				   * based on num_resv_q
				   */
	DP_F_DEV_RESV_SCH = BIT(1), /*!< Reserve scheduler for this dev
				     * based on num_resv_sched
				     */
	DP_F_DEV_CONTINUOUS_Q = BIT(2), /*!< Reserve continuous Q for this dev
					 * DP_F_DEV_RESV_Q bit should also be
					 * set for this continuous Q alloc
					 */
};

/**
 * @brief dp_gpid_tx_info
 */
/* FIXME: ??? Not sure need or not */
#define DP_DFT_MAX_PKT_LEN 1600 /*!< Default maximal packet length */
/* FIXME: ??? Not sure need or not */
#define DP_DFT_MIN_PKT_LEN 60 /*!< Default minimal packet length */
/*! @addtogroup Datapath_Driver_Structures */
/*! @brief  PPA Sub-interface Data structure
 *@param port_id  Datapath Port Id corresponds to PMAC Port Id
 *@param subif    Sub-interface Id info. In GRX500, this 15 bits,
 *                only 13 bits in PAE are handled [14, 11:0]
 *\note
 */

enum DP_RXOUT_QOS_MODE {
	DP_RXOUT_BYPASS_QOS_ONLY = 0, /*!< bypass QOS but with FSQM */
	DP_RXOUT_QOS, /*!< with QOS */
	DP_RXOUT_BYPASS_QOS_FSQM, /*!< bypass QOS and FSQM */
	DP_RXOUT_QOS_MAX /*!< Not valid RXOUT qos mode */

};

struct dp_dev_data_priv {
	/*User given data for dp_register_dev() */
	struct dp_dev_data *udata;

	/*!< number of intermediate CQM dequeue port: DC/SPL->CQM->IPPU.
	 * it is for DC device only and not valid for other non-DC devices
	 * RX IPPU
	 */
	int num_ippu;

	/*!< rx_ippu ring */
	struct dp_ring_pktdeq ippu[DP_MAX_INTERM];
};

/*! @brief struct dp_mem_port_info, which used for DirectConnected (DC) */
struct dp_mem_port_info {
	/*!< [in] number of DC PktEnq/RxOut ring */
	u8 num_enq;

	/*!< [in] number of DC BufReq/RxIn ring */
	u8 num_req;

	/*!< [in] number of DC PktDeq/TxIn ring */
	u8 num_deq;

	/*!< [in] number of DC BufReq/Txout ring */
	u8 num_ret;

	/*!< need umt or not, this is per cqm port(not per ring) config
	 *  Set by top level drivers: valid for memory port
	 */
	bool umt_enable;

	/*!< umt period setting for memory igp port, set by top level driver
	 *   valid only if umt_enable flag is true and in UMT msg mode, ie,
	 *  non-interurpt mode
	 */
	u16 umt_igp_interval;

	/*!< umt period setting for memory egp port, set by top level driver
	 *   valid only if umt_enable flag is true and in UMT msg mode, ie,
	 *  non-interurpt mode
	 */
	u16 umt_egp_interval;

	/*!< enable CQM buffer meta data marking, set by top level driver*/
	bool enable_cqm_meta;

	/* in topaz, by default 1 DC device will use 1 memory port only
	 * But it can use multiple rings for pktdeq/enq, bufrequest/retrun
	 */

	/*!< dc pkt deq*/
	struct dp_ring_pktdeq deq[DP_NUM_DC_PKTDEQ_RING];

	/*!< dc buffer return*/
	struct dp_ring_bufret ret[DP_NUM_DC_BUFRET_RING];

	/*!< dc pkt enqueue*/
	struct dp_ring_pktenq enq[DP_NUM_DC_PKTENQ_RING];

	/*!< dc buffer request*/
	struct dp_ring_bufreq req[DP_NUM_DC_BUFREQ_RING];
};

/*! @brief struct dp_dev_data, which used for DirectConnected (DC)
 *  applications
 */
struct dp_dev_data {
	/*!< flag operation, for device registration. refer to enum DP_DEV_DATA_FLAG
	 */
	enum DP_DEV_DATA_FLAG flag_ops;

	/* ---------------------------------------------------
	 * variables will be used by CQM driver: Begin
	 * ---------------------------------------------------
	 */

	/*!< DC memory port information. Caller need to allocate it if needed*/
	struct dp_mem_port_info *mem_port;

	u32 bm_policy_res_id; /*!< [in] buffer policy id */
	int tx_policy_base; /* [out] tx policy id base */
	int tx_policy_num; /* [out] tx policy num */

	/*!< [in] optional parameters */
	struct dp_dev_opt_param opt_param;

	/*!< [out] lookup_sel configuration */
	struct lookup_sel_attr sel[CQM_LOOKUP_SEL_NUM];
	/* ---------------------------------------------------
	 * variables will be used by CQM driver: End
	 * ---------------------------------------------------
	 */
	/*!< gpid extra information*/
	struct dp_gpid_tx_info gpid_info;
	u16 max_subif;  /*!< [in] maximum subif required for this dp port */
	u16 max_gpid;  /*!< [in] maximum subif required which will be mapped to
			* PP continuous GPID block. The continuous limitation is
			* from GSWIP (subif->GPID mapping design), not because
			* of PP itself.
			* since overall number of GPID < nubmer of CTP in HW,
			* DP need to add this parameter to fully use of shared
			* HW resource.
			*/
	int num_resv_q; /*!< [in] reserve the required number of queues. Valid
			 *   only if DP_F_DATA_RESV_Q bit valid in \ref flag_ops
			 */
	int num_resv_sched; /*!< [in] reserve required number of schedulers.
			     *   Valid only if DP_F_DATA_RESV_SCH bit valid in
			     *   \ref flag_ops
			     */
	u16 qos_resv_q_base; /*!< [out] PPv4 QoS reserved Q base */
	u32 qos_id; /*!< [in] qos configuration id to match dts setting */
	u16 gpid_base; /*!< [out] return gpid_base to caller, eg: PON driver */

};

/*! @brief enum DP_LATE_INIT_TYPE */
enum DP_LATE_INIT_TYPE
{
	DP_LATE_INIT_MIB_COUNTER = 0,
	DP_LATE_INIT_MAX,
};

/*! @addtogroup Datapath_Driver_API */
/*! @brief  Datapath Allocate Datapath Port aka PMAC port
 *	port may map to an exclusive netdevice like in the case of
 *	ethernet LAN ports. In other cases like WLAN, the physical port is a
 *	Radio port, while netdevices are Virtual Access Points (VAPs)
 *	In this case, the  AP netdevice can be passed
 *Alternately, driver_port & driver_id will be used to identify this port
 *@param[in] owner  Kernel module pointer which owns the port
 *@param[in] dev pointer to Linux netdevice structure (optional), can be NULL
 *@param[in] dev_port Physical Port Number of this device managed by the driver
 *@param[in] port_id Optional port_id number requested. Usually 0 means dynamic
 *      allocation by dpm, otherwise as specified by caller during registration.
 *      For de-registration, port_id must be the same value as returned during
 *      registration. Note, negative value is not valid and never should happen.
 *@param[in] pmac_cfg PMAC related configuration parameters
 *@param[in] flags :Various special Port flags like WAVE500, VRX318 etc ...
 *	-  DP_F_DEALLOC_PORT :Deallocate the already allocated port
 *@return  Returns dp_port id, -1(DP_FAILURE) on error
 */
int32_t dp_alloc_port(struct module *owner, struct net_device *dev,
		      u32 dev_port, int32_t port_id,
		      dp_pmac_cfg_t *pmac_cfg, uint32_t flags);

/*! @brief  Datapath Allocate Datapath Port aka PMAC port
 *	port may map to an exclusive netdevice like in the case of
 *	ethernet LAN ports. In other cases like WLAN, the physical port is a
 *	Radio port, while netdevices are Virtual Access Points (VAPs)
 *	In this case, the  AP netdevice can be passed
 *Alternately, driver_port & driver_id will be used to identify this port
 *@param[in] inst the DP instance, start from 0. At SOC side, it is always 0.
 *           For pherpheral device, normally it is non-zero.
 *@param[in] owner  Kernel module pointer which owns the port
 *@param[in] dev pointer to Linux netdevice structure (optional), can be NULL
 *@param[in] dev_port Physical Port Number of this device managed by the driver
 *@param[in] port_id Optional port_id number requested. Usually 0 means dynamic
 *      allocation by dpm, otherwise as specified by caller during registration.
 *      For de-registration, port_id must be the same value as returned during
 *      registration. Note, negative value is not valid and never should happen.
 *@param[in] pmac_cfg PMAC related configuration parameters
 *@param[in,out] data to pass the peripheral information.
 *      Valid for registration only and NULL for de-registration
 *@param[in] flags :Various special Port flags like WAVE500, VRX318 etc ...
 *	-  DP_F_DEALLOC_PORT :Deallocate the already allocated port
 *@return  Returns dp_port id, -1(DP_FAILURE) on error
 */
int32_t dp_alloc_port_ext(int inst, struct module *owner,
			  struct net_device *dev,
			  u32 dev_port, int32_t port_id,
			  dp_pmac_cfg_t *pmac_cfg,
			  struct dp_port_data *data,
			  uint32_t flags);

/*! @brief  Higher layer Driver Datapath registration API
 *@param[in] owner  Kernel module pointer which owns the port
 *@param[in] port_id Port Id returned by alloc() function
 *@param[in] dp_cb  Datapath driver callback structure
 *@param[in] flags :Special input flags to alloc routine
 *		- F_DEREGISTER :Deregister the device
 *@return 0 - OK / -1 - Correct Return Value
 *@note
 */
int32_t dp_register_dev(struct module *owner, uint32_t port_id,
			dp_cb_t *dp_cb, uint32_t flags);

/*! @brief  Higher layer Driver Datapath registration API
 *@param[in] inst the DP instance, start from 0. At SOC side, it is always 0.
 *           For pherpheral device, normally it is non-zero.
 *@param[in] owner  Kernel module pointer which owns the port
 *@param[in] port_id Port Id returned by alloc() function
 *@param[in] dp_cb  Datapath driver callback structure
 *@param[in,out] data to pass the peripheral information
 *      Valid for registration only and NULL for de-registration
 *@param[in] flags :Special input flags to alloc routine
 *		- F_DEREGISTER :Deregister the device
 *@return 0 - OK / -1 - Correct Return Value
 *@note
 */
int32_t dp_register_dev_ext(int inst, struct module *owner,
			    u32 port_id,
			    dp_cb_t *dp_cb,
			    struct dp_dev_data *data,
			    uint32_t flags);

/*! @brief  Allocates datapath subif number to a sub-interface netdevice
 *Sub-interface value must be passed to the driver
 *port may map to an exclusive netdevice like in the case of ethernet LAN ports
 *@param[in] owner  Kernel module pointer which owns the port
 *@param[in] dev pointer to Linux netdevice structure, only for VRX318 driver,
 *it can be NULL. All other driver's, must provide valid dev pointer.
 *@param[in] subif_name pointer
 *@param[in,out] subif_id pointer to subif_id structure including port_id
 *@param[in] flags :
 *	DP_F_DEREGISTER - De-register already registered subif/vap
 *@return Port Id  / IFX_FAILURE
 *@note
 */
int32_t dp_register_subif(struct module *owner, struct net_device *dev,
			  char *subif_name, dp_subif_single_t *subif_id,
			  uint32_t flags);

/*! @brief  Allocates datapath subif number to a sub-interface netdevice
 *Sub-interface value must be passed to the driver
 *port may map to an exclusive netdevice like in the case of ethernet LAN ports
 *@param[in] inst the DP instance, start from 0. At SOC side, it is always 0.
 *           For pherpheral device, normally it is non-zero.
 *@param[in] owner  Kernel module pointer which owns the port
 *@param[in] dev pointer to Linux netdevice structure, only for VRX318 driver,
 *it can be NULL. All other driver's, must provide valid dev pointer.
 *@param[in] subif_name pointer
 *@param[in,out] subif_id pointer to subif_id structure including port_id
 *@param[in,out] data to pass the peripheral information
 *@param[in] flags :
 *	DP_F_DEREGISTER - De-register already registered subif/vap
 *@return Port Id  / IFX_FAILURE
 *@note
 */
int32_t dp_register_subif_ext(
	int inst,
	struct module *owner,
	struct net_device *dev,
	char *subif_name,
	dp_subif_single_t *subif_id,
	struct dp_subif_data *data,
	uint32_t flags);

/*! @brief  Allocates datapath subif number for special sub-interface netdevice
 *Sub-interface value must be passed to the driver
 *port may map to a special netdevice like a CPU or reinsertion port
 *@param[in] inst the DP instance, start from 0. At SOC side, it is always 0.
 *           For pherpheral device, normally it is non-zero.
 *@param[in] dev pointer to Linux netdevice structure, only for VRX318 driver,
 *           it can be NULL. All other drivers, must provide valid dev pointer.
 *@param[in] subif_name pointer
 *@param[in,out] subif_id pointer to subif_id structure including port_id
 *@param[in,out] data to pass the peripheral information. data->flag_ops
 *               supports DP_SUBIF_CPU and DP_SUBIF_REINSERT.
 *@param[in] flags :
 *	DP_F_DEREGISTER - De-register already registered subif/vap
 *@return 0 - OK / FAILURE
 *@note
 */
int32_t dp_register_subif_spl_dev(int inst, struct net_device *dev,
				  char *subif_name, dp_subif_t *subif_id,
				  struct dp_subif_data *data, uint32_t flags);

/*! @brief  Transmit packet to low-level Datapath driver
 *@param[in] rx_if  Rx If netdevice pointer - optional
 *@param[in] rx_subif  Rx rx_subif netdevice pointer
 *@param[in] skb  pointer to packet buffer like sk_buff.
 *           note, skb is freed if return value is not DP_TX_FN_BUSY
 *@param[in] len    Length of packet to transmit
 *@param[in] flags :refer to \enum DP_TX_FLAGS
 *@return Legacy mode:0(DP_SUCCESS) on succeed / -1(DP_FAILURE) on error
 *        advanced mode: 0(DP_TX_FN_CONSUMED) on succeed / -1(DP_FAILURE) on error /
 *                        DP_TX_FN_BUSY if lower HW is busy
 *@note  dp_xmit will be in advanced return mode if flags bit DP_TX_NEWRET set
 */
int32_t dp_xmit(struct net_device *rx_if, dp_subif_single_t *rx_subif,
		struct sk_buff *skb, int32_t len, uint32_t flags);

/*! @brief  Get Pkt dst-if Sub-if value, default queue/rings, ...
 *	Sub-interface value must be passed to the driver
 *	port may map to an exclusive netdevice like in the case of ethernet
 *	LAN ports.
 *@param[in] netif  pointer to stack network interface structure
  *@param[in] subif_data pointer to subif_data structure, which will be used if
  *          netif NULL. The common use case is for DSL ATM PPPOA case.
 *@param[in] skb pointer to sk_buff structure that carries packet destination
 *	information
 *@param[in] dst_mac  Destiantion MAC address to which packet is addressed
 *@param[in] flags :Reserved
 return dp_subif_t pointer if OK / NULL if error
 *@note  Prototype of PPA_DP_GET_NETIF_SUBIF function. Not implemented in PPA
 *	Datapath, but in client driver like WAVE500 WLAN driver
 *@note  Either skbuff parameters to be used  or dst_mac to determine subifid
 *	For WAVE driver, this will be the StationId + VAP on the basis of
 *	the dst mac. This function is only to be used by the PPA to program
 *	acceleration entries. The client driver is still expected to fill
 *	in Sub-interface id when transmitting to the underlying datapath driver
 */
dp_subif_t *dp_get_netif_subifid(struct net_device *netif, struct sk_buff *skb,
				 void *subif_data,
				 uint8_t dst_mac[DP_MAX_ETH_ALEN],
				 uint32_t flags);

/*! @brief  free subifid buffer which was returned by dp_get_netif_subifid.
 *  @param[in] subif  buffer to be freed
 */

void dp_free_netif_subifid(dp_subif_t *subif);

/*! @brief  get port information
 *@param[in] inst instance id
 *@param[in] port_id dp port id
 *@param[out] port attribute
 *@return 0 if OK / -1 if error
 *@note: these two API is for dp_dbg and pon mib counter special use.
 */
int dp_get_port_prop(int inst, int port_id, struct dp_port_prop *prop);
int dp_get_subif_prop(int inst, int port_id, int vap, struct dp_subif_prop *prop);

/*! @brief  Check if given interface is a pmapper
 *@param[in] dev  pointer to stack network interface structure
 *@return true if the interface is a pmapper; false otherwise
 */
bool dp_is_pmapper_check(struct net_device *dev);

/*! @brief  The API is for CQM to send received packets(skb) to dp lib. Datapath
 *	lib will do basic packet parsing and forwards it to related drivers,\n
 *	like ethernet driver, wifi and lte and so on. Noted.
 *	It is a chained skb and dp lib will split it before send it to
 *	related drivers
 *@param[in] skb  pointer to packet buffer like sk_buffer
 *@param[in] flags  reserved for futures
 *@return 0 if OK / -1 if error
 */
int32_t dp_rx(struct sk_buff *skb, uint32_t flags);
/*!
 *@brief  The API is for configuing PMAC based on deque port
 *@param[in] port  Egress Port
 *@param[in] pmac_cfg Structure of ingress/egress parameters for setting PMAC
 *	   configuration
 *@return 0 if OK / -1 if error
 */

enum DP_F_STATS_ENUM {
	DP_F_STATS_SUBIF = 1 << 0, /*!< Flag to get network device subif
				    *   mib counter
				    */
	DP_F_STATS_PAE_CPU = 1 << 1 /*!< Flag to get CPU network mib counter*/
};

/*!
 *@brief  The API is for getting MIB counters
 *@param[in] dev pointer to Linux netdevice structure, it will be used if
 *           subif_id NULL.
 *@param[in] subif_id pointer to subif_id structure including port_id
 *@param[in] path_stats stats for path
 *@param[in] flags  reserved for futures
 *@return 0 if OK / -1 if error
 */
int dp_get_netif_stats(struct net_device *dev, dp_subif_single_t *subif_id,
		       struct rtnl_link_stats64 *path_stats, uint32_t flags);

/*!
 *@brief  The API is for dp_clear_netif_stats
 *@param[in] dev pointer to Linux netdevice structure, it will be used if
 *           subif_id NULL.
 *@param[in] subif_id pointer to subif_id structure including port_id
 *@param[in] flag  reserved for futures
 *@return 0 if OK / -1 if error
 */
int dp_clear_netif_stats(struct net_device *dev, dp_subif_single_t *subif_id,
			 uint32_t flag);

/*!
 *@brief  The API is for dp_get_port_subitf_via_ifname
 *@param[in] ifname  Interface Name
 *@return dp_subif_t pointer if OK / NULL if error
 */
dp_subif_t *dp_get_port_subitf_via_ifname(char *ifname);

/*!
 *@brief  The API is for dp_get_port_subitf_via_dev
 *@param[in] dev pointer to Linux netdevice structure, only for VRX318 driver,
 *@param[in,out] subif pointer to subif_id structure including port_id
 *@return 0 if OK / -1 if error
 */
dp_subif_t *dp_get_port_subitf_via_dev(struct net_device *dev);
#if IS_ENABLED(CONFIG_DPM_DATAPATH_CPUFREQ)

/*!
 *@brief  The API is for dp_get_port_subitf_via_dev
 *@param[in,out] new_state pointer to structure ltq_cpufreq_threshold,
 *@param[in] flag flag
 *@return 0 if OK / -1 if error
 */
int dp_coc_new_stat_req(int new_state, uint32_t flag);

/*!
 *@brief  The API is for dp_get_port_subitf_via_dev
 *@param[in,out] threshold pointer to structure ltq_cpufreq_threshold,
 *@param[in] flags flags
 *@return 0 if OK / -1 if error
 */
/*! DP's submodule to call it */
/*int dp_set_rmon_threshold(struct dp_coc_threshold *threshold,
 *			    uint32_t flags);
 */
#endif /*! CONFIG_DPM_DATAPATH_CPUFREQ*/

/*! get port flag. for TMU proc file cat /proc/tmu/queue1 and /proc/tmu/eqt */
u32 get_dp_port_flag(int k);
char *get_dp_port_type_str(int k);
int get_dp_port_type_str_size(void);
u32 *get_port_flag(int inst, int index);

/*! @brief set pmac configuration
 *@param[in] inst DP instance ID
 *@param[in] port DP port ID
 *@param[in] pmac_cfg point of pmac configuration need to set
 *@return 0 if OK / -1 if error
 */
int dp_pmac_set(int inst, u32 port, dp_pmac_cfg_t *pmac_cfg);

/*! struct dp_umt_cap_mode: dp UMT capability */
struct dp_umt_cap_mode {
	u32 enable:1;  /*!< UMT enable flag: 0-disable 1-eanbled */
	u32 rx_accumulate:1; /*!< rx accumulated supported flag: 1 supported */
	u32 rx_incremental:1; /*!< rx incremental supported flag: 1 supported */
	u32 tx_accumulate:1; /*!< tx accumulated supported flag: 1 supported */
	u32 tx_incremental:1; /*!< tx incremental supported flag: 1 supported */
};

/*! struct dp_umt_cap: dp UMT capability */
struct dp_umt_cap {
	struct dp_umt_cap_mode umt_hw_auto; /*!< HW UMT self mode */
	struct dp_umt_cap_mode umt_hw_user; /*!< HW UMT: user mode */
	struct dp_umt_cap_mode umt_sw; /*!< SW UMT mode */
};

#define DP_MAX_NAME  20 /*!< max name length in character */
#define DP_MAX_GSWIP_LOGICAL_MODE 3 /*!< Max GSWIP Logical Mode */

/*! struct dp_cap: dp capability per instance */
struct dp_cap {
	int inst; /*!< Datapath instance id */

	u32 tx_hw_chksum:1;  /*!< output: HW checksum offloading support flag
			      *   for tx path
			      *   0 - not support
			      *   1: support
			      */
	u32 rx_hw_chksum:1;  /*!< output: HW checksum verification support flag
			      *   for rx path
			      *   0 - not support
			      *   1: support
			      */
	u32 hw_tso: 1; /*!< output: HW TSO offload support for TX path */
	u32 hw_gso: 1; /*!< output: HW GSO offload support for TX path */
	u32 hw_ptp: 1; /*!< HW PTP support */
	u32 max_cpu: 8;			/*!< Maximum number of CPU */
	u32 max_port_per_cpu: 8;	/*!< Maximum number of ports per CPU */
	char qos_eng_name[DP_MAX_NAME]; /*!< QOS engine name in string */
	char pkt_eng_name[DP_MAX_NAME]; /*!< Packet Engine Name String */
	int max_num_spl_conn; /*!< max number of special connection */
	int max_num_queues; /*!< max number of QOS queue supported */
	int max_num_scheds; /*!< max number of QOS scheduler supported */
	int max_num_deq_rings; /*!< max number of CQM dequeue ring */
	int max_num_qos_ports; /*!< max number of QOS dequeue port */
	int max_num_dp_ports; /*!< max number of dp port */
	int max_num_subif_per_port; /*!< max number of subif per dp_port */
	int max_num_subif; /*!< max number of subif supported. Maybe no meaning?
			    */
	u32 max_eth_port; /*!< Number of MAC Ports */
	u32 max_virt_eth_port; /*!< Number of maximum logical ports over PMAC */
	struct dp_umt_cap umt; /*!< UMT cap */
};

/*!
 *@brief  The API is for dp_get_cap
 *@param[in,out] cap dp_cap pointer, caller must provide the buffer
 *@param[in] flag for future
 *@return 0 if OK / -1 if error
 */
int dp_get_cap(struct dp_cap *cap, int flag);

/*!
 *@brief  The API is for dp_get_module_owner
 *@param[in] ep dp_port ID
 *@return module owner pointer if success, otherwise NULL
 */
struct module *dp_get_module_owner(int ep);

/*!
 *@brief  Set the minimum frame length on the DP Port
 *@param[in] dp_port dp_port ID
 *@param[in] min_frame_len minimal frame size for this DP port ID
 *@param[in] flag Reserved
 *@return return 0 if OK / -1 if error
 */

int dp_set_min_frame_len(s32 dp_port,
			 s32 min_frame_len,
			 uint32_t flags);

/*!
 *@brief  Enable/Disable forwarding RX packet to specified netif or ifname
 *@param[in] netif netowrk device pointer. if NULL, then check ifname
 *@param[in] ifname if netif == NULL
 *@param[in] rx_enable: 1 enable rx for this device
 *@param[in] flag:
 *            DP_RX_ENABLE: enable rx, ie, allow forwarding rx pkt to this dev
 *            DP_RX_DISABLE: stop rx, ie, DP should drop rx pkt for this dev
 *@return return 0 if OK / -1 if error
 */
#define DP_RX_ENABLE  1
#define DP_RX_DISABLE 0
int dp_rx_enable(struct net_device *netif, char *ifname, uint32_t flags);

/*!
 *@brief Datapath Manager Pmapper Configuration Set
 *@param[in] dev: network device point to set pmapper
 *@param[in] mapper: buffer to get pmapper configuration
 *@param[in] flag: reserve for future
 *@return Returns 0 on succeed and -1 on failure
 *@note  for pcp mapper case, all 8 mapping must be configured properly
 *       for dscp mapper case, all 64 mapping must be configured properly
 *       def ctp will match non-vlan and non-ip case
 *	For drop case, assign CTP value == DP_PMAPPER_DISCARD_CTP
 */
int dp_set_pmapper(struct net_device *dev, struct dp_pmapper *mapper, u32 flag);

/*!
 *@brief Datapath Manager Pmapper Configuration Get
 *@param[in] dev: network device point to set pmapper
 *@param[out] mapper: buffer to get pmapper configuration
 *@param[in] flag: reserve for future
 *@return Returns 0 on succeed and -1 on failure
 *@note  for pcp mapper case, all 8 mapping must be configured properly
 *       for dscp mapper case, all 64 mapping must be configured properly
 *       def ctp will match non-vlan and non-ip case
 *	 For drop case, assign CTP value == DP_PMAPPER_DISCARD_CTP
 */
int dp_get_pmapper(struct net_device *dev, struct dp_pmapper *mapper, u32 flag);

/*!
 *@brief Datapath Manager DMA RX IRQ enable/disable API
 *@param[in] inst: DP instance ID
 *@param[in] ch: DMA RX Channel information
 *@param[in] flag: reserve for future
 *@return Returns DP_SUCCESS on succeed and DP_FAILURE on failure
 */
int dp_dma_chan_irq_rx_enable(int inst, struct dp_dma_ch *ch, int flag);

/*! @brief struct dp_spl_cfg_priv, which is used for dpm and CQM driver only
 */
struct dp_spl_cfg_priv {
	/*User given data stored here*/
	struct dp_spl_cfg *udata;
	/* number of intermedimum dequeue port ring: CQM->IPPU
	 * Voice: 1
	 * TOE driver (TSO): 1
	 * VPN-A:  0 ?
	 * APP lite: 0
	 * PP NF: 1
	 */
	u8 num_tx_ippu;

	/* number of intermedimum dequeue port ring: CQM->IPPU
	 * Voice: 1
	 * TOE driver (TSO): 0
	 * VPN-A:  1
	 * APP lite: 0
	 * PP NF: 0
	 */
	u8 num_rx_ippu;

	/* intermediate dequeue port from CPU->CQM->IPPU->Device */
	struct dp_ring_pktdeq tx_ippu[DP_MAX_SPLCONN_INTERM_RING];

	/* intermediate dequeue port from Device->CQM->IPPU */
	struct dp_ring_pktdeq rx_ippu[DP_MAX_SPLCONN_INTERM_RING];
};

/*! @brief struct dp_spl_cfg, which is used for register/register special
 *  CPU path applications
 */
struct dp_spl_cfg {
	u32 flag; /*!< [in]DE_REGISTER for de-registration, otherwise for
		   *       registration
		   */
	enum DP_SPL_TYPE type; /*!< [in] Special Connectity Type */
	u32 spl_id; /*!< [out] for dp_spl_conn_get() only
		     *   [out/in] For registration, it is [out] parameter
		     *            For de-registration, it is [in] parameter
		     *            Note: for LGM, DP can support up to 8 special
		     *                  CPU connection path at present.
		     *            From CQM point of view, may from 16 better.
		     */
	u32 f_subif : 1; /*!< [out] for dp_spl_conn_get() only
			  *   [in] need allocate subif flag
			  *        LGM:
			  *         Even if caller disable it. DP internally
			  *         still assign one subif for it.
			  *         caller just skip its output parameters
			  *         For Voice: 1
			  *         For TOE(TSO/LRO): 0
			  *         For VPN-A Driver: 1
			  *         For APP Litepath: 1
			  *         For PP NF: Fragmenter: 1
			  */
	u32 f_gpid : 1; /*!< [out] for dp_spl_conn_get() only
			 *   [in] need allocate GPID flag
			 *        Even if caller disable it. DP internally will
			 *        assign one GPID for it. But DP will not
			 *        call PP API to really configure it in this
			 *        case. Valid only if @f_subif valid
			 *       LGM:
			 *         For Voice: 1
			 *         For TOE(TSO/LRO): 0
			 *         For VPN-A Driver: 1
			 *         For APP Litepath: 1
			 *         For PP NF: 1
			 */
	u32 f_policy : 1; /*!< [out] for dp_spl_conn_get() only
			   *   [in] need allocate policy or not
			   *   LGM:
			   *     voice: 1 Need allocate one special policy,
			   *     TOE(LRO/TSO): 0 CQM driver configure it to
			   *                  system policy for TSO/LRO_ACK
			   *     VPN-A Driver: 0, ie, reuses input buffer
			   *     App Litepath: 0, ie, reuses the CPU DQ
			   *                   ports for the host
			   *     PP NF: Fragmenter NF: 0
			   */
	u32 f_hostif : 1; /*!< [out] for dp_spl_conn_get() only
			   *   [in] need create PP hostif flag
			   *   LGM:
			   *     voice: 1
			   *     TOE(LRO/TSO): 0
			   *     VPN-A Driver: 1
			   *     App Litepath: 1
			   *     PP NF: Mutlicast : 1,
			   *            Others: 0
			   */
	u32 prel2_len:2; /*!< [out] for dp_spl_conn_get() only
			   *   [in] size of PMAC header in multiple of 16 bytes
			   *   LGM:
			   *     0: Disabled
			   *     1: 16 bytes
			   *     2: 32 bytes
			   *     3: 48 bytes
			   */
	int subif; /*!< [out] subif id. Valid only if f_subif is set */
	int gpid; /*!< [out] gpid. Valid only if f_gpid is set */
	int spl_gpid; /*!< [out] gpid for voice. Valid only if f_gpid is set */
	dp_cb_t *dp_cb; /*!< [out] for dp_spl_conn_get() only
			 *   [in] for subif level callback.
			 *        Not for GRX500/PRX300
			 *        LGM:
			 *         *)For rx_fn of dp_cb
			 *            voice: yes for rx_fn
			 *            TOE(LRO/TSO): no
			 *            VPN-A Driver: yes
			 *            App Litepath: yes
			 *            PP NF: no
			 *         *)For tx_fn of dp_cb
			 *            voice: no
			 *            TOE(LRO/TSO): yes for TSO
			 *            VPN-A Driver: no
			 *            App Litepath: no
			 *            PP NF: Multicast NF needs rx_cb
			 */
	struct net_device *dev; /*!< [out] for dp_spl_conn_get() only
				 *   [in] network device if need.
				 *   if rx_fn of dp_cb valid, dev should be
				 *      valid also
				 *   Not for GRX500/PRX300
				 *   LGM:
				 *     voice: yes
				 *     TOE(LRO/TSO)- Not required (No Dequeue )
				 *     VPN-A Driver: yes
				 *     App Litepath: yes
				 *     Fragmenter NF: Not required
				 */
	int dp_port; /*!< [out] dp_port ID, normally it is CPU 0.
		      *   if -1, then not applicable for this special connect
		      */

	/* [out[ mem_port memory port information.
	 * top level network driver need to allocate memory for it
	 * num_enq number of DC PktEnq/RxOut ring:
	 *   Voice: 1 ring based on 1 legacy CPU port.
	 *          In the LGM, it is QOSbyass.
	 *          Here need configure qmapping table
	 *   TOE driver(TSO): 1 for DMA enqueue
	 *   VPN-A:  1 ring based on DC port ?? Need align later
	 *   APP lite: 0
	 *   PP NF: 0
	 * num_req number of DC BufReq/RxIn ring
	 *   Voice: 0 ?
	 *   TOE driver: 0 ?
	 *   VPN-A:  0
	 *   APP lite: 0
	 *   PP NF: 0
	 * num_deq number of DC PktDeq/TxIn ring
	 *   Voice: 1 ring based on 1 legacy CPU port.
	 *   TOE driver(LRO): 1 ring based on 1 CPU port
	 *   VPN-A:  1 ring based on DC port
	 *   APP lite: 0
	 *   PP NF: 1 ring based on 1 CQM deq port
	 * num_ret number of DC BufReq/Txout ring
	 *   Voice: 0 ?
	 *   TOE driver: 0 ?
	 *   VPN-A:  0
	 *   APP lite: 0
	 *   PP NF: 0
	 */
	struct dp_mem_port_info *mem_port;
};

/*!
 *@brief Datapath Manager Initialize Speical Connectivity API
 *@param[in] inst: DP instance ID
 *@param[in] conn: Special Connect information
 *@param[in] flag: for deregistration, it needs to use flag DP_F_DEREGISTER
 *                 otherwise it is for registration a new CPU special path
 *@return Returns DP_SUCCESS on succeed and DP_FAILURE on failure
 */
int dp_spl_conn(int inst, struct dp_spl_cfg *conn);

#define DP_SPL_CONN_MAX_CNT 4 /*!< maximum number of struct dp_spl_cfg */

/*!
 *@brief Datapath Manager get Speical Connectivity API
 *@param[in] inst: DP instance ID
 *@param[in] type: DP special connectivity type
 *@param[in/out] conns: user allocated buffer to store connectivity info
 *@param[in] cnt: number of dp_spl_cfg been allocated
 *@return returns number of results been wrote, 0 means not found
 */
int dp_spl_conn_get(int inst, enum DP_SPL_TYPE type,
		    struct dp_spl_cfg *conns, u8 cnt);

enum DP_EVENT_TYPE_BITS {
	DP_EVENT_INIT_BIT,
	DP_EVENT_ALLOC_PORT_BIT,
	DP_EVENT_DE_ALLOC_PORT_BIT,
	DP_EVENT_REGISTER_DEV_BIT,
	DP_EVENT_DE_REGISTER_DEV_BIT,
	DP_EVENT_REGISTER_SUBIF_BIT,
	DP_EVENT_DE_REGISTER_SUBIF_BIT,
	DP_EVENT_MAX_BIT
};

#define EVENT(TYPE) BIT(DP_EVENT_##TYPE##_BIT)

/*! @brief enum DP_EVENT_TYPE to define the different kind of event type
 */
enum DP_EVENT_TYPE {
	DP_EVENT_INIT = EVENT(INIT), /*!< event callback for initiazation
					*   when first network driver register to dp via
					*   calling API dp_alloc_port_ext
					*   DP will call event callback before
					*   allocate port for this request
					*/
	DP_EVENT_ALLOC_PORT = EVENT(ALLOC_PORT), /*!< event callback for
					*   alloc port
					*/
	DP_EVENT_DE_ALLOC_PORT = EVENT(DE_ALLOC_PORT), /*!< event callback
					*   for de_alloc port
					*/
	DP_EVENT_REGISTER_DEV = EVENT(REGISTER_DEV), /*!< event callback for
					*   register dev
					*/
	DP_EVENT_DE_REGISTER_DEV = EVENT(DE_REGISTER_DEV), /*!< event callback
					*   for de_register dev
					*/
	DP_EVENT_REGISTER_SUBIF = EVENT(REGISTER_SUBIF), /*!< event callback
					*   for register subif
					*/
	DP_EVENT_DE_REGISTER_SUBIF = EVENT(DE_REGISTER_SUBIF), /*!< event
					*   callback for de_register subif
					*/
	DP_EVENT_MAX = EVENT(MAX)
};

/*! @brief struct dp_event_init_info, init event specific information
 */
struct dp_event_init_info {
	struct module *owner; /*!< Kernel module pointer which owns the port */
	struct net_device *dev; /*!< network device pointer. it can be NULL for
				 *   some device
				 */
	u32 dev_port; /*!< Physical Port Number of this device managed by the
		       *   driver
		       */
};

/*! @brief struct dp_event_alloc_info, alloc port event specific information
 */
struct dp_event_alloc_info {
	struct module *owner; /*!< Kernel module pointer which owns the port */
	struct net_device *dev; /*!< network device pointer. it can be NULL for
				 *   some device
				 */
	u32 dev_port; /*!< Physical Port Number of this device managed by the
		       *   driver
		       */
};

/*! @brief struct dp_event_reg_dev_info, register dev event specific
 *  information
 */
struct dp_event_reg_dev_info {
	struct module *owner; /*!< Kernel module pointer which owns the port */
	struct net_device *dev; /*!< network device pointer. it can be NULL for
				 *   some device
				 */
	int dpid;  /*!< dp port ID */
	struct dp_dev_data *dev_data; /*!< dev data info */
};

/*! @brief struct dp_event_reg_subif_info, register subif event specific
 *   information
 */
struct dp_event_reg_subif_info {
	int dpid;  /*!< dp port ID */
	s32 subif; /*!< device subif */
	struct net_device *dev; /*!< subif network device pointer.
				 *   It can be NULL for some case, like ATM DSL
				 */
};

/*! @brief struct dp_dev_event, which is used for dp_register_event
 */
struct dp_event_info {
	int inst; /*!< [out] dp instance id */
	enum DP_EVENT_TYPE type; /*!< [out] event type */
	union {
		struct dp_event_init_info init_info; /*!< initialization info */
		struct dp_event_alloc_info alloc_info; /*!< alloc port info */
		struct dp_event_alloc_info de_alloc_info; /*!< de_alloc port
							   * info
							   */
		struct dp_event_reg_dev_info reg_dev_info; /*!< register
							    *   dev info
							    */
		struct dp_event_reg_dev_info dereg_dev_info; /*!< de_register
							      *   dev info
							      */
		struct dp_event_reg_subif_info reg_subif_info;/*!< register
							       *   subif
							       *   info
							       */
		struct dp_event_reg_subif_info de_reg_subif_info;/*!< register
								  *   subif
								  *   info
								  */
	};
	int alloc_flags;  /*!< dp port alloc flags */
	void *data; /*!< [out] pass back the data to caller which was provied by
		     *   caller when calling dp_register_event_cb API
		     */

};

enum DP_EVENT_OWNER {
	DP_EVENT_OWNER_OTHERS = 0, /*!< Event owner type OTHERS */
	DP_EVENT_OWNER_PPA, /*!< Event owner type PPA */
	DP_EVENT_OWNER_MIB, /*!< Event owner type MIB */
	DP_EVENT_OWNER_MAX /*!< Maximum owners types */
};

/* Note:this callback should should support tasklet/softtimer related context */
struct dp_event {
	int inst; /*!< [in] dp instance id */
	enum DP_EVENT_OWNER owner; /*!< [in] owner who register to DP
				    *   for event status monitoring.
				    *   For debugging purpose only
				    */
	enum DP_EVENT_TYPE type; /*!< [in] type of event */
	int f_owner_only; /*!< [in] flag to indicate whether only limited event
			   *   will be trigger. It is mainly for DPDK if
			   *   DPDK only interted in those data port which owned
			   *   by DPDK only
			   */
	int32_t(*dp_event_cb)(struct dp_event_info *info);  /*!< [in] event
							     *   callback
							     *   function
							     *   callback contex
							     *   can be tasklet
							     *   or soft-timer
							     */
	void *data; /*!< [in] optinal data which will be passed backed to
		     *   callback function when event triggered
		     */
	void *id; /*!< [in/out] handle for dp_register_event_cb.
		   *   When set as NULL, it will be treated as [out] parameter.
		   *   Upon sucessful registration of the callback function, a
		   *   valid handle will be return.
		   *   The handle can be used for subsequent update by
		   *   registered apps. It will be used as [in] parameter.
		   *   All existing values will be overwritten by the new
		   *   values in dp_event_info.
		   */
};

/*!
 *@brief dp_register_event_cb API to register event callback functions
 *@param[in/out] info: registration event info
 *@param[in] flag: DP_F_DEREGISTER is for de-registration,
 *                 otherwise it is for registration
 *@return DP_SUCCESS on succeed and DP_FAILURE on failure
 */
int dp_register_event_cb(struct dp_event *info, uint32_t flag);

/*!
 *@brief dp_set_ethtool_stats_fn API to register callback function which
 * retrieves counters for ethtool statistics
 *@param[in] inst: dp instance id
 *@param[in] cb: pointer to function called to retrieve counters for
 *		 ethtool statistics
 */
void dp_set_ethtool_stats_fn(int inst,
			     void (*cb)(struct net_device *dev,
					struct ethtool_stats *stats,
					u64 *data));

/*!
 *@brief dp_set_ethtool_stats_strings_cnt_fn API to register callback function which
 * retrieves retrieve number of counters
 *@param[in] inst: dp instance id
 *@param[in] cb: pointer to function called to retrieve number of counters for
 *		 ethtool statistics
 */
void dp_set_ethtool_stats_strings_cnt_fn(int inst,
					 int (*cb)(struct net_device *dev));

/*!
 *@brief dp_set_ethtool_stats_strings_fn API to register callback function which
 * retrieves set of counters' names
 *@param[in] inst: dp instance id
 *@param[in] cb: pointer to function called to retrieve set of counters' names
 *		 for ethtool statistics
 */
void dp_set_ethtool_stats_strings_fn(int inst,
				     void (*cb)(struct net_device *dev, u8 *data));

/*! @brief Enumerator DP_OPS_TYPE */
enum DP_OPS_TYPE {
	DP_OPS_CQM = 0, /*!< CQM ops type */
	DP_OPS_QOS, /*!< QOS ops type */
	DP_OPS_IPPU, /*!< IPPU ops type */
	DP_OPS_EPPU, /*!< IPPU ops type */
	DP_OPS_UMT, /*!< UMT ops type */
	DP_OPS_VPN, /*!< VPN ops type */
	DP_OPS_BM, /*!< BM ops type */
	DP_OPS_CNT,  /*!< total ops type count */
};

/*! @brief DP_DEPENDENCY_BITS: define dependency on some key module.
 *  later need to add IPPU/EPPU
 */
#define DP_DEPENDENCY_BITS (BIT(DP_OPS_CQM) | BIT(DP_OPS_QOS))

/*!
 *@brief Datapath Manager ops registration
 *@param[in] inst: DP instance ID
 *@param[in] type: ops type
 *@param[in] ops: pointer to ops structure
 *@note  set to NULL to deregister
 *@return return 0 if OK / -1 if error
 */
int dp_register_ops(int inst, enum DP_OPS_TYPE type, void *ops);

/*!
 *@brief get ops registration
 *@param[in] inst: DP instance ID
 *@param[in] type: ops type
 *@return ops pointer if registered, or NULL if not registered
 */
void *dp_get_ops(int inst, enum DP_OPS_TYPE type);

/*!
 * @brief get network device's MTU
 * @param[in] dev: network device pointer
 * @param[out] mtu_size: return the maximum MTU can be supported
 *                       for this device based on current HW configuration
 * @return DP_SUCCESS on succeed and DP_FAILURE on failure
 */
int dp_get_mtu_size(struct net_device *dev, u32 *mtu_size);

/*!
 * @brief set network device's MTU
 * @param[in] dev: network device pointer
 * @param[in] mtu_size: new Linux network MTU requested
 * @return DP_SUCCESS on succeed and DP_FAILURE on failure
 */
int dp_set_mtu_size(struct net_device *dev, u32 mtu_size);

/*!
 *@brief free Rx/Tx buffer
 *@param[in] info: buffer info
 *@param[in] flag: reserved
 *@return DP_SUCCESS on success and DP_FAILURE on failure
 */
int dp_free_buffer_by_policy(struct dp_buffer_info *info, u32 flag);

/*!
 *@brief get UMT ops registration
 *@param[in] inst: DP instance ID
 *@return UMT ops pointer if registered, or NULL if not registered
 */
static inline struct umt_ops *dp_get_umt_ops(int inst)
{
	return (struct umt_ops *)dp_get_ops(inst, DP_OPS_UMT);
}

#define DP_DMA_PORT_BIT_POS 16
#define DP_DMA_CTRL_BIT_POS 24

/*!
 *@brief is_stream_port
 *@param[in] inst     : alloc flags
 *@param[out] true    : if stream port
 *@param[out] false   : if not stream port
 */
static inline bool is_stream_port(int flags)
{
	return !!(flags & (DP_F_FAST_ETH_LAN | DP_F_FAST_ETH_WAN | DP_F_GINT |
			   DP_F_GPON));
}

/*!
 *@brief is_dc_port
 *@param[in] inst     : alloc flags
 *@param[out] true    : if dc port
 *@param[out] false   : if not dc port
 */
static inline bool is_dc_port(int flags)
{
	return !!(flags & DP_F_ACA);
}

#define DP_CQM_LU_MODE_GET BIT(1)

/*!
 *@brief dp_lookup_mode_cfg API
 *@param[in] inst	: Instance
 *@param[in] sel	: qmap values, to set/get lookup mode
 *@param[in] flag	: DP_CQM_LU_MODE_GET
 *@return DP_SUCCESS on succeed and DP_FAILURE on failure
 */
int dp_lookup_mode_cfg(int inst, struct cqm_lookup_sel *sel,
		       u32 flag);

/*!
 *@brief dp_net_dev_get_ss_stat_strings API - function used by ethtool
 *	 to retrieve set of counters' names
 *@param[in] dev: net device
 *@param[out] data: buffer pointer where the function will copy counters' names
 */
void dp_net_dev_get_ss_stat_strings(struct net_device *dev, u8 *data);

/*!
 *@brief dp_net_dev_get_ss_stat_strings_count API - function used by ethtool
 *	 to retrieve number of counters
 *@param[in] dev: net device
 *@return number of counters
 */
int dp_net_dev_get_ss_stat_strings_count(struct net_device *dev);

/*!
 *@brief dp_net_dev_get_ethtool_stats API - function used by ethtool
 *	 to retrieve counters' values
 *@param[in] dev: net device
 *@param[out] stats: for dumping NIC-specific statistics
 *@param[out] data: for dumping counters values sequence
 */
void dp_net_dev_get_ethtool_stats(struct net_device *dev,
				  struct ethtool_stats *stats, u64 *data);

struct dp_dc_res {
	int inst;    /*! <in> dp instance */
	int dp_port; /*! <in> dp port id/lpid/ep */
	int res_id;  /*! <in> resource ID */
	struct dp_dc_rx_res rx_res;  /*! <out> rx resource */
	struct dp_dc_tx_res tx_res;  /*! <out> tx resource */
};

int dp_get_dc_config(struct dp_dc_res *res, int flag);

/*!
 *@brief dp_set_br_vlan_limit API, will initialize Bridge VLAN
 *with how many vlanid and bridgeid supported per bridge
 *@param[in] br_dev: Specifies bridge device to set the limit
 *@param[in] maxvlanid: Specifies Maximum VLAN ID supported
 *@return DP_SUCCESS on succeed and DP_FAILURE on failure
 */
int dp_set_br_vlan_limit(struct net_device *br_dev, u32 maxvlanid);

/*!
 *@brief struct dp_subblk_info to pass PCE Sub-Block Information
 */
struct dp_subblk_info {
	int portid;   		/*!< [in] LPID, Used only for CTP Region */
	int subif;    		/*!< [in] SubifIdGroup/VAP, Used only
				 * for CTP Region
				 */
	int bp;			/*!< [in] BP identifier, Used for this subblk
				 * if pmapper device, need set proper bp value,
				 * otherwise must set to zero
				 */
	bool subblk_protected;	/*!< [in] 1 - Sub-block is protected
			         * delete will be done by user
			         * 0 - Sub-block is Un-Protected,
			         * delete will be done once last rule gets
			         * deleted
			         */
	int subblk_id;		/*!< [in/out] Sub-Block Type ID
				 *   while add ID will be returned back to user
				 *   While del, ID need to be passed to API
				 */
	int subblk_firstidx;	/*!< [in] If not <=0, use this as first Idx to
				 *   allocate
				 */
	int subblk_size;	/*!< [in] Sub-Block Size */
	char subblk_name[32];  /*!< [in] Name for sub-block */
	int prio;		/*!< [in] Rule priority inside sub-block */
};

/*!
 *@brief dp_get_tx_cqm_pkt API, get CPU CQM TX counter via port_id and
 *       subifid group
 *@param[in] inst: dp instance
 *@param[in] dp_port: dp port id
 *@param[subif_id_grp] subifid group or vap
 *@return CPU TX counter on succeed, otherise return 0
 */
u32 dp_get_tx_cqm_pkt(int inst, int dp_port, int subif_id_grp);

/*!
 *@brief dp_is_ready API, check whether DPM initialize is done or not
 *@return true if DPM full initialization almost done, otherwise return false;
 */
bool dp_is_ready(u32 *state);

int dp_hostif_update(int inst, int dpid, int vap, struct dp_hif_datapath *new_dp);

/* tc callback */
int dp_ndo_setup_tc(struct net_device *dev, enum tc_setup_type type,
		    void *type_data);
/* tc dev capability update */
int dp_dev_update_tc(struct net_device *dev);

/* toe dev capability update */
int dp_dev_update_toe(struct net_device *dev);

/* xfrm callback */
struct xfrm_state;
int dp_xdo_dev_state_add(struct xfrm_state *x);
void dp_xdo_dev_state_delete(struct xfrm_state *x);
bool dp_xdo_dev_offload_ok(struct sk_buff *skb, struct xfrm_state *x);
/* xfrm dev capability update */
int dp_dev_update_xfrm(struct net_device *dev);

/* ptp */
struct ifreq;
int dp_get_ts_info(struct net_device *dev,
		   struct ethtool_ts_info *ts_info);
int dp_ndo_do_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd);

#endif /*DATAPATH_API_H */
