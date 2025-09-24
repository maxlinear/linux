// SPDX-License-Identifier: GPL-2.0
/*****************************************************************************
 * Copyright (c) 2024, MaxLinear, Inc.
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.

*******************************************************************************/

#ifndef DATAPATH_H
#define DATAPATH_H
#include <linux/types.h>
#include <asm/byteorder.h>
#include <linux/skbuff.h>	/*skb */
#include <linux/types.h>
#include <linux/netdevice.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/version.h>
#include <uapi/linux/if.h> /* for IFNAMSIZ */

#if IS_ENABLED(CONFIG_QOS_MGR)
#include <linux/soc/mxl/qos_mgr/qos_mgr_tc_hook.h>
#endif
#include <linux/percpu.h>
#include <linux/version.h>

#include <linux/soc/mxl/pp_qos_api.h>
#if IS_ENABLED(CONFIG_MXL_CQM_SKB) || \
	LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0)
	#define DP_SKB_HACK
#endif
#include <linux/soc/mxl/datapath_api.h>
#include <linux/soc/mxl/datapath_api_qos.h>
#include <linux/soc/mxl/datapath_inst.h>
#include <linux/soc/mxl/datapath_gdb_wrapper.h>

#define DP_DEBUGFS_PATH "/sys/kernel/debug/dp"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
#define call_rcu_bh call_rcu
#endif /* LINUX_VERSION_CODE */
#define DP_NOT_USE_NETDEV_REGISTER 1
#define DP_OPS_HACK 1 /* be default to enable it for PPA to hack ndo_xxx in some ops */

/* DPM build bug on for compile time assert on condition*/
#define DPM_BUILD_BUG_ON(cond, msg) typedef u8 msg[(cond) ? -1:0]
/* Instead of BUG_ON(), we defined our own, to use under dis-optimization*/
#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
#define DPM_BUG_ON(cond, ...) do {\
	if (cond) { \
		int *null_ptr = NULL; \
		pr_err("\nDPM: %s %d trigger manual panic here\n\n", __func__, \
			__LINE__); \
		dump_stack(); \
		*null_ptr = 1; \
	} \
} while (0)
#else
#define DPM_BUG_ON(...)
#endif

struct dp_gswip {
	struct timer_list	timer;
	unsigned long		ageout;
	u32			version;
	u32			features;
	/* These bits are different on NetCP NU Switch ALE */
	u32			port_mask_bits;
	u32			port_num_bits;
	unsigned long		*p0_untag_vid_mask;
};

/* Note: below macro should equal or bigger than HW real capability
 *       since it is used for array size
 */
#define MAX_SUBIFS 256 /* Max subif per DPID */
#define MAX_DP_PORTS 16

/* to do: need to tune these parameter later */
#define DP_MAX_PPV4_PORT 256
#define DP_MAX_QUEUE_NUM 512

/* maximum CQM dequeue ring */
#define DP_MAX_CQM_DEQ 128

#if DP_MAX_CQM_DEQ >= DP_MAX_PPV4_PORT
#error "Wrong macro definition: PP QOS port should more than CQM Dequeue ring"
#endif

#define MAX_Q_PER_RING	65 /* Maximum queue per port */

/* Max value for the CQM IGP ports 256 has put for timebeing*/
#define DP_MAX_CQM_ENQ			256
#define DP_MAX_CQM_RET			128
#define DP_MAX_CQM_REQ			128

/*Get subif from the ADP descriptor of 32 bit wide*/
#define ADP_GET_SUBIF(desc_val, off, mask) ((desc_val >> off) & mask) //TODO: VBOLLA use these macros in the GPID
/*Set subif into the ADP descritpor of 32 bit wide*/
#define ADP_SET_SUBIF(subif, off, mask) ((((u32)subif) & mask) << off)//TODO: VBOLLA

/* DP_EMULATE_MAX_NODES will be passed via ccflag in DPM qemu SDK env */
#ifdef DP_EMULATE_MAX_NODES
#define DP_MAX_NODES DP_EMULATE_MAX_NODES
#else
#define DP_MAX_NODES 2048 /* Maximum PPV4 nodes */
#endif

#define DP_MAX_PER_CPU_GPID 		1
#define DP_MAX_CPU_GPID 		(DP_MAX_PER_CPU_GPID * CQM_MAX_CPU)
/* Number of Subif for Normal CPU path: 1 subif per CPU, 2 CQM deq rings per CPU*/
#define DP_MAX_CPU_SUBIF		DP_MAX_CPU_GPID
#define DP_MAX_DPID0_SUBIF		16
#define DP_MAX_SPL_CONN 		(DP_MAX_DPID0_SUBIF - DP_MAX_CPU_SUBIF)
#define DP_MAX_DPID0_GPID		DP_MAX_DPID0_SUBIF
#define DP_DPID0_SUBIF_START		0
#define DP_DPID0_SUBIF_END		(DP_DPID0_SUBIF_START + DP_MAX_DPID0_SUBIF - 1)
#define DP_DPID0_CPU_SUBIF_START	(DP_MAX_DPID0_SUBIF - DP_MAX_CPU_SUBIF)
#define DP_DPID0_CPU_SUBIF_END		(DP_MAX_DPID0_SUBIF - 1)
#define DP_DPID0_SPL_CONN_SUBIF_START	DP_DPID0_SUBIF_START
/* Number of PP_NF Special connnections.
 * Note: each special connection reserve 1 subif only, and no GPID
 * TPZ: reassemble NF, fragment NF, TCP ack, IPsec NF, LLD NF, TSO/LRO = Total 6
 *      f_gpid = 0 for topaz.
 */
#define MAX_PP_NF_CNT 			6

#define DP_MAX_RING_PER_CPU 		2

#define DP_PLATFORM_DE_INIT BIT(0)

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DISABLE_OPTIMIZE)
#define DP_NO_OPTIMIZE_PUSH \
	_Pragma("GCC push_options") \
	_Pragma("GCC optimize (\"O0\")")
#define DP_NO_OPTIMIZE_POP _Pragma("GCC pop_options")
#else
#define DP_NO_OPTIMIZE_PUSH
#define DP_NO_OPTIMIZE_POP
#endif

#define MAX_TC_ID 15 /* maximum traffic class ID */

#define UP_STATS(atomic) atomic_add(1, &(atomic))

#define STATS_GET(atomic) atomic_read(&(atomic))
#define STATS_SET(atomic, val) atomic_set(&(atomic), val)
#define DP_CB(i, x) dp_port_prop[i].info.x
#define get_cqm_inst(dp_inst) dp_port_prop[dp_inst].cqm_inst

#define dp_set_val(reg, val, mask, offset) do {\
	(reg) &= ~(mask);\
	(reg) |= (((val) << (offset)) & (mask));\
} while (0)

#define dp_get_val(val, mask, offset) (((val) & (mask)) >> (offset))

#define DP_DEBUG_ASSERT(expr, fmt, arg...)  do { if (expr) \
	pr_err(fmt, ##arg); \
} while (0)

extern u64 dp_dbg_flag;
extern u64 dp_dbgfs_flag;
extern u32 dp_dbg_err;
extern int dp_dbg_mode;
extern char *log_buf;
extern int log_buf_len;

void dp_trace_pr(unsigned long ip, const char *fmt, ...);

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
/* DP_DUMP no __func__ inserted */
#define DP_DUMP(fmt, arg...) do { \
	if (!dp_dbg_mode) \
		pr_info(fmt, ##arg); \
	else \
		dp_trace_pr(_THIS_IP_, fmt, ##arg); \
} while (0)

#define DP_DUMP_CONT(fmt, arg...) do { \
	if (!dp_dbg_mode) \
		pr_cont(fmt, ##arg); \
	else \
		dp_trace_pr(_THIS_IP_, fmt, ##arg); \
} while (0)

/* DP_INFO will insert "DPM: __func__" */
#define DP_INFO(fmt, arg...) DP_DUMP("DPM: %s: " fmt, __func__, ##arg)

/* _DP_DEBUG will not insert "DPM: __func__:" with specified print catagory */
#define _DP_DEBUG(flags, fmt, arg...)  do { \
	if (unlikely(dp_dbg_flag & (flags))) { \
		DP_DUMP(fmt, ##arg); \
	} \
} while (0)

/* DP_DEBUG will insert "DPM: __func__:" with specified print catagory */
#define DP_DEBUG(flags, fmt, args...) \
	_DP_DEBUG(flags, "DPM: %s: " fmt, __func__, ##args)

#else /* CONFIG_DPM_DATAPATH_DBG */
#define DP_INFO(fmt, arg...)
#define DP_DUMP(fmt, arg...)
#define DP_DEBUG(flags, fmt, arg...)
#define _DP_DEBUG(flags, fmt, arg...)
#endif /* end of CONFIG_DPM_DATAPATH_DBG */

extern bool dp_mod_exiting;
extern int dflt_cpu_vap[DP_MAX_INST];
extern int dflt_q_cpu[DP_MAX_INST];
extern int dflt_q_drop[DP_MAX_INST];

#define MAX_GPID		128
#define MAX_GPID_PER_PORT	16   /* Maybe can be further reduced */
#define MAX_SUBIF_PER_PORT	128  /* GPON up to 128 */

#define CPU_PORT		0
#define CPU_SUBIF		0 /* CPU default subif ID*/

static inline bool is_invalid_port(int port_id)
{
	if ((port_id < 0) || (port_id >= MAX_DP_PORTS)) {
		pr_err("DPM: port_id(%d) out of 0~%d\n", port_id, MAX_DP_PORTS -1);
		return true;
	}
	return false;
}

static inline bool is_invalid_inst(int inst)
{
	if ((inst < 0) || (inst >= DP_MAX_INST)) {
		pr_err("DPM: inst(%d) out of 0~%d\n", inst, DP_MAX_INST-1);
		return true;
	}
	return false;
}

#define DP_MAX_HW_CAP 1

#ifdef DP_SPIN_LOCK
#define DP_LOCK_T spinlock_t
#define DP_LOCK_INIT(lock) spin_lock_init(lock)
#define DP_DEFINE_LOCK(lock) DEFINE_SPINLOCK(lock)
#define DP_LIB_LOCK    spin_lock_bh
#define DP_LIB_UNLOCK  spin_unlock_bh
#else
#define DP_LOCK_T struct mutex
#define DP_LOCK_INIT(lock) mutex_init(lock)
#define DP_DEFINE_LOCK(lock) DEFINE_MUTEX(lock)
#define DP_LIB_LOCK    mutex_lock
#define DP_LIB_UNLOCK  mutex_unlock
#endif

extern DP_LOCK_T dp_lock;

#define CHECK_BIT(var, pos) (((var) & (1 << (pos))) ? 1 : 0)

enum dp_xmit_errors {
	DP_XMIT_ERR_DEFAULT = 0,
	DP_XMIT_ERR_NOT_INIT,
	DP_XMIT_ERR_IN_IRQ,
	DP_XMIT_ERR_NULL_SUBIF,
	DP_XMIT_ERR_PORT_TOO_BIG,
	DP_XMIT_ERR_NULL_SKB,
	DP_XMIT_ERR_NULL_IF,
	DP_XMIT_ERR_REALLOC_SKB,
	DP_XMIT_ERR_EP_ZERO,
	DP_XMIT_ERR_GSO_NOHEADROOM,
	DP_XMIT_ERR_CSM_NO_SUPPORT,
	DP_XMIT_PTP_ERR,
};

enum dp_message_errors {
	DP_ERR_SUBIF_NOT_FOUND = -7,
	DP_ERR_INIT_FAIL       = -6,
	DP_ERR_INVALID_PORT_ID = -5,
	DP_ERR_MEM             = -4,
	DP_ERR_NULL_DATA       = -3,
	DP_ERR_INVALID_SUBIF   = -2,
	DP_ERR_DEFAULT         = -1,
};

/*! Registration status flag */
enum PORT_FLAG {
	PORT_FREE = 0,		/*! The port is free */
	PORT_ALLOCATED,		/*! the port is already allocated to others,
				 * but not registered or no need to register.\n
				 * eg, LRO/CAPWA, only need to allocate,
				 * but no need to register
				 */
	PORT_DEV_REGISTERED,	/*! dev Registered already. */
	PORT_SUBIF_REGISTERED,	/*! subif Registered already. */

	PORT_FLAG_NO_VALID	/*! Not valid flag */
};

#define DP_DBG_ENUM_OR_STRING(name, value, short_name) {name = value}

enum DP_DBG_FLAG {
	DP_DBG_FLAG_DBG                = BIT_ULL(0),
	DP_DBG_FLAG_DUMP_RX_DATA       = BIT_ULL(1),
	DP_DBG_FLAG_DUMP_RX_DESCRIPTOR = BIT_ULL(2),
	DP_DBG_FLAG_DUMP_RX            = (BIT_ULL(1) | BIT_ULL(2)),
	DP_DBG_FLAG_DUMP_TX_DATA       = BIT_ULL(3),
	DP_DBG_FLAG_DUMP_TX_DESCRIPTOR = BIT_ULL(4),
	DP_DBG_FLAG_DUMP_TX_SUM        = BIT_ULL(5),
	DP_DBG_FLAG_DUMP_TX            = (BIT_ULL(3) | BIT_ULL(4) | BIT_ULL(5)),
	DP_DBG_FLAG_COC                = BIT_ULL(6),
	DP_DBG_FLAG_MIB                = BIT_ULL(7),
	DP_DBG_FLAG_MIB_ALGO           = BIT_ULL(8),
	DP_DBG_FLAG_CQM_BUF            = BIT_ULL(9),
	DP_DBG_FLAG_PAE                = BIT_ULL(10),
	DP_DBG_FLAG_INST               = BIT_ULL(11),
	DP_DBG_FLAG_DEV                = BIT_ULL(12),
	DP_DBG_FLAG_NOTIFY             = BIT_ULL(13),
	DP_DBG_FLAG_LOGIC              = BIT_ULL(14),
	DP_DBG_FLAG_QOS                = BIT_ULL(15),
	DP_DBG_FLAG_QOS_DETAIL         = BIT_ULL(16),
	DP_DBG_FLAG_LOOKUP             = BIT_ULL(17),
	DP_DBG_FLAG_REG                = BIT_ULL(18),
	DP_DBG_FLAG_OPS                = BIT_ULL(19),
	DP_DBG_FLAG_QMAP               = BIT_ULL(20),
	DP_DBG_FLAG_SPL                = BIT_ULL(21),
	DP_DBG_FLAG_INIT_STAT          = BIT_ULL(22),
	DP_DBG_FLAG_GDB                = BIT_ULL(23),
	DP_DBG_FLAG_HOOK               = BIT_ULL(24),

	/*Note, once add a new entry here in the enum,
	 *need to add new item in below macro DP_DBG_FLAG_LIST
	 */
	DP_DBG_FLAG_MAX = BIT_ULL(63)
};

enum DP_DBGFS_FLAG {
	DP_DBGFS_FLAG_DPID   = BIT_ULL(0),
	DP_DBGFS_FLAG_PORT   = BIT_ULL(1),
	DP_DBGFS_FLAG_DEV    = BIT_ULL(2),
	DP_DBGFS_FLAG_DEVOPS = BIT_ULL(3),
	DP_DBGFS_FLAG_DEQ    = BIT_ULL(5),
	DP_DBGFS_FLAG_ENQ    = BIT_ULL(6),
	DP_DBGFS_FLAG_QOS    = BIT_ULL(7),
	DP_DBGFS_FLAG_HOOKS  = BIT_ULL(8),
	DP_DBGFS_FLAG_DBGFS  = BIT_ULL(0) | BIT_ULL(1) |
			       BIT_ULL(2) | BIT_ULL(3) |
			       BIT_ULL(4) | BIT_ULL(5) |
			       BIT_ULL(6) | BIT_ULL(7) |
			       BIT_ULL(8),
	DP_DBGFS_FLAG_MAX    = BIT_ULL(63)
};

/*Note: per bit one variable */
#define DP_DBG_FLAG_LIST {\
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DBG, "dbg"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DUMP_RX_DATA, "rx_data"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DUMP_RX_DESCRIPTOR, "rx_desc"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DUMP_RX, "rx"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DUMP_TX_DATA, "tx_data"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DUMP_TX_DESCRIPTOR, "tx_desc"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DUMP_TX_SUM, "tx_sum"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DUMP_TX, "tx"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_COC, "coc"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_MIB, "mib"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_MIB_ALGO, "mib_algo"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_CQM_BUF, "cqm_buf"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_PAE, "pae"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_INST, "inst"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_DEV, "dev"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_NOTIFY, "notify"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_LOGIC, "logic"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_QOS, "qos"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_QOS_DETAIL, "qos2"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_LOOKUP, "lookup"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_REG, "register"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_OPS, "ops"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_QMAP, "qmap"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_SPL, "spl"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_INIT_STAT, "init"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_GDB, "gdb"), \
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_HOOK, "hook"), \
	/*must be last one */\
	DP_DBG_ENUM_OR_STRING(DP_DBG_FLAG_MAX, "")\
}

#define DP_DBGFS_FLAG_LIST {\
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_DPID, "dbgfs_dpid"), \
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_PORT, "dbgfs_port"), \
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_DEV, "dbgfs_dev"), \
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_DEVOPS, "dbgfs_devops"), \
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_DEQ, "dbgfs_deq"), \
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_ENQ, "dbgfs_enq"), \
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_QOS, "dbgfs_qos"), \
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_HOOKS, "dbgfs_hooks"), \
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_DBGFS, "dbgfs"), \
	/*must be last one */\
	DP_DBGFS_ENUM_OR_STRING(DP_DBGFS_FLAG_MAX, "")\
}

enum QOS_FLAG {
	NODE_LINK_ADD = 0, /*add a link node */
	NODE_LINK_GET,     /*get a link node */
	NODE_LINK_EN_GET,  /*Get link status: enable/disable */
	NODE_LINK_EN_SET,  /*Set link status: enable/disable */
	NODE_UNLINK,       /*unlink a node: in fact, it is just flush now*/
	LINK_ADD,          /*add a link with multiple link nodes */
	LINK_GET,          /*get a link may with multiple link nodes */
	LINK_PRIO_SET,     /*set arbitrate/priority */
	LINK_PRIO_GET,     /*get arbitrate/priority */
	QUEUE_CFG_SET,     /*set queue configuration */
	QUEUE_CFG_GET,     /*get queue configuration */
	SHAPER_SET,        /*set shaper/bandwidth*/
	SHAPER_GET,        /*get shaper/bandwidth*/
	NODE_ALLOC,        /*allocate a node */
	NODE_FREE,         /*free a node */
	NODE_CHILDREN_FREE,  /*free all children under one specified parent:
			      *   scheduler/port
			      */
	DEQ_RING_RES_GET,  /*get all full links under one specified ring */
	COUNTER_MODE_SET,  /*set counter mode: may only for TMU now so far*/
	COUNTER_MODE_GET,  /*get counter mode: may only for TMU now so far*/
	QUEUE_MAP_GET,     /*get lookup entries based on the specified qid*/
	QUEUE_MAP_SET,     /*set lookup entries to the specified qid*/
	NODE_CHILDREN_GET, /*get direct children list of node*/
	QOS_LEVEL_GET,     /* get Max Scheduler level for Node */
	QOS_Q_LOGIC,       /* get logical queue ID based on physical queue ID */
	QOS_GLOBAL_CFG_GET, /* get global qos config info */
	QOS_PORT_CFG_SET, /* set qos port config info */
	QOS_BLOCK_FLUSH_PORT, /* Block and Flush all QiD's in the port */
	QOS_BLOCK_FLUSH_QUEUE, /* Block and Flush particular QiD */
	CODEL_GET,	/* get system level codel configuration */
	CODEL_SET,	/* set system level codel configuration */
};

enum DP_MAP_Q_TYPE {
	DP_MAP_Q_SUBIF = 0, /*!< to this subif's queue OR we say to the device */
	DP_MAP_Q_CPU,  /*!< to CPU 1st default queue */
	DP_MAP_Q_RX_IPPU0, /*!< For RX ippu queue(non-frag) for sop_eop:2bit  = 0b11 (binary)*/
	DP_MAP_Q_RX_IPPU1, /*!< For RX ippu queue(fragment) for sop_eop:2bit != 0b11 (binary)
			    * ie 0b00, 0b01, 0b10
			    */
	DP_MAP_Q_TX_IPPU, /*!< For TX ippu queue map */
	DP_MAP_Q_DROP, /*!< For reset queue mapping during de_regsiter_subif */
};

#define Q_MAP_F_DONT_CARE -1

struct dp_q_map_entry {
	struct dp_q_map_sel_pair sel[CQM_LOOKUP_SEL_NUM];
	enum DP_MAP_Q_TYPE q_type;
};

struct q_map_info {
	char *name; /* name for this set of rules */
	int num; /* number of q_map rules */
	u32 alloc_flag; /* its alloc_flag of this dp_port */
	struct dp_q_map_entry *entry; /* cqm lookup entry rules */
};

struct dev_mib {
	atomic_t rx_fn_rxif_pkt; /*! received packet counter */
	atomic_t rx_fn_txif_pkt; /*! transmitted packet counter */
	atomic_t rx_fn_dropped; /*! transmitted packet counter */
	atomic_t tx_cqm_pkt; /*! transmitted packet counter */
	atomic_t tx_clone_pkt; /*! duplicate unicast packet for cloned flag */
	atomic_t tx_hdr_room_pkt; /*! duplicate pkt for no enough headerroom*/
	atomic_t tx_tso_pkt;	/*! transmitted packet counter */
	atomic_t tx_pkt_dropped;	/*! dropped packet counter */
};

struct mib_global_stats {
	u64 rx_rxif_pkts;
	u64 rx_txif_pkts;
	u64 rx_rxif_clone;
	u64 rx_drop;
	u64 tx_pkts;
	u64 tx_drop;
};

DECLARE_PER_CPU_SHARED_ALIGNED(struct mib_global_stats, mib_g_stats);

#define MIB_G_STATS_INC(member) do { \
			per_cpu(mib_g_stats, get_cpu()).member++; \
			put_cpu(); \
		} while(0)

#define MIB_G_STATS_RESET(member, cpu) do { \
			per_cpu(mib_g_stats, cpu).member = 0; \
		} while(0)

#define MIB_G_STATS_GET(member, cpu) \
			per_cpu(mib_g_stats, cpu).member

#ifdef TOPAZ_CODE_ENABLE
struct dp_igp {
	u32 igp_id; /* CQM enqueue port based ID */
	u32 igp_dma_ch_to_gswip; /* DMA TX channel base to GSWIP */
	u32 num_out_cqm_deq_port; /* num of CQM dequeue port to GSWIP */
};

struct dp_egp {
	int egp_id; /* EGP port ID */
	enum DP_EGP_TYPE type; /* EGP port: DP_EGP_TO_DEV, DP_EGP_TO_GSWIP */
};
#endif

struct qos_cqm_info {
#if (defined(__BYTE_ORDER) && (__BYTE_ORDER == __BIG_ENDIAN)) || defined(__BIG_ENDIAN)
	u16 num;  /* the nunber of continuous DEq port/ring */
	u16 base;  /* CQM Deq Port(LGM/PRX)/Ring(TPZ) base */
#else
	u16 base;  /* CQM Deq Port(LGM/PRX)/Ring(TPZ) base */
	u16 num; /* the nunber of continuous DEq port/ring */
#endif
};

struct qos_setting_match {
	char *catagory;
	u32 flag;
	int idx_catagory;
};

struct dp_qos_setting {
	bool valid;
	const char *node_name;
	const char *category;
	u32 flag;
	union {
		u32 qos_id;
		struct qos_cqm_info cqm_deq;
	};
	u32 wred_en;
	u32 codel_en;
	u32 qlen;

	/* for debug only */
	u16 category_idx;
	u16 sub_category_idx;
};

enum DP_ADP_TEMPL_TYPE {
	TEMPL_TX_NORMAL = 0, /**
			      * To EPPU: No checksum, make proper templ at initial API
			      *    For streaming port: normal GPID, egflag=1, cksum=0
			      *    For DC port:           SPL_GPID, egflag=1, cksum=0
			      */
	TEMPL_TX_CKSUM,   /**
			   * To TX IPPU: checksum only.
			   *    Streaming/DC ports:    SPL_GPID, egflag=1, cksum=1
			   */
	TEMPL_TX_PTP, 	 /**
			  * PTP: most probably direct goto EPPU, TODO: VBOLLA: need to check
			  */
	TEMPL_TX_PTP_CKSUM, //PTP: most probably direct goto TX_IPPU, not sure? who does the cksum?
	TEMPL_RX_NORMAL, /*only valid for mask, template value is not used.*/
	TEMPL_MAX
};

/*! Sub interface detail information */
struct dp_subif_info {
	s32 flags;
	u32 subif;
	u8 dp_port;
	struct net_device *netif; /*! pointer to  net_device */
	char device_name[IFNAMSIZ]; /*! devide name, like wlan0, */
	struct dev_mib mib; /*! mib */
	struct net_device *ctp_dev; /*CTP dev for PON pmapper case*/
	u8 deq_ring_idx; /* To store deq port relative index from register_subif */
	u32 dw3[TEMPL_MAX]; /*adp dw3 template used for gpid update*/

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
	int subif_num; /*!< valid subif/ctp num.
			*   output for dp_get_netif_subifid,
			*   no use for dp_register_subif_ext
			*/
	long subif_cmn_end;
	/*!< --------------------------------------------------------------
	 *!< end  of placeholder for fast copy subif_info.
	 *  -----------------------------------------------------------------
	 */

	s16 q_node[DP_MAX_DEQ_PER_DEV]; /* logical Q node Id if applicable */
	s16 qos_deq_port[DP_MAX_DEQ_PER_DEV]; /* qos port id */
	s16 cqm_deq_ring[DP_MAX_DEQ_PER_DEV]; /* CQM physical dequeue port ID
					       * (absolute)
					       */
	u8 num_deq_ring;
	s16 tx_ippu_q_node[DP_MAX_INTERM]; /* logical Q node Id if applicable */
	s16 tx_ippu_qos_deq_port[DP_MAX_INTERM]; /* tx ippu qos port id */
	s16 tx_ippu_ring[DP_MAX_INTERM];
	u8 num_tx_ippu;
	/*rx IPPU is only for ACA case*/
	s16 rx_ippu_q_node[DP_MAX_INTERM]; /* logical Q node Id if applicable */
	s16 rx_ippu_qos_deq_port[DP_MAX_INTERM]; /* rx ippu qos port id */
	s16 rx_ippu_ring[DP_MAX_INTERM];
	u8 num_rx_ippu;
	u32 subif_flag; /* To store original flag from caller during
			 * dp_register_subif
			 */
	atomic_t rx_flag; /* To enable/disable DP rx */
	atomic_t f_dfl_eg_sess[DP_DFL_SESS_NUM]; /*! flag to indicate whether
					       *  dfl_eg_sess valid or
					       *  not
					       */
	u16 max_pkt_size;
	u16 cqm_mtu_size;
	u16 headroom_size;
	u16 tailroom_size;
	u16 spl_gpid_headroom_size;
	u16 spl_gpid_tailroom_size;
	u16 min_pkt_len;
	int min_pkt_len_cfg;
	u16 tx_policy_base;    /* TX policy base */
	u8 tx_policy_num;      /* TX policy number */
	u8 tx_policy_map;      /* TX policy map */
	u16 rx_policy_base; /* RX policy base */
	u8 rx_policy_num;   /* RX policy number */
	u8 rx_policy_map;   /* RX policy map */
	u8 pkt_only_en;
	u8 seg_en;
	dp_rx_fn_t rx_fn;	/*!< Rx function callback */
	dp_get_netif_subifid_fn_t get_subifid_fn; /*! get subif ID callback */
	enum DP_SPL_TYPE spl_conn_type; /* only for special path,
					 * otherwise set to DP_SPL_INVAL
					 */
	int tx_pkt_credit;
	struct __dp_spl_conn_cfg_db *spl_cfg;
	const struct dp_port_info *port_info; /* points to port_info */
	struct q_map_info *q_map; /* If NULL, then map to drop queue */
	u16 last_hosif_qid[DP_MAX_CPU][DP_MAX_HOSTIF];
	struct dp_dev *dp_dev; /* link to dp_dev */
};

enum DP_PRIV_F {
	DP_PRIV_PER_CTP_QUEUE = BIT(0), /*Manage Queue per CTP/subif */
};

/* this macro should be >= max(DP_NUM_DC_PKTENQ_RING, DP_SPLCONN_PKTENQ_RING) */
#define DP_MAX_PKTENQ_RING 2
#if (DP_MAX_PKTENQ_RING < DP_NUM_DC_PKTENQ_RING) || \
    (DP_MAX_PKTENQ_RING < DP_SPLCONN_PKTENQ_RING)
#error "wrong DP_MAX_PKTENQ_RING value"
#endif

/* this macro should be >= max(DP_NUM_DC_PKTDEQ_RING, DP_SPLCONN_PKTDEQQ_RING,
 * DP_NUM_PKTDEQ_RING)
 */
#define DP_MAX_PKTDEQ_RING DP_NUM_PKTDEQ_RING
#if (DP_MAX_PKTDEQ_RING < DP_NUM_DC_PKTDEQ_RING)  || \
    (DP_MAX_PKTDEQ_RING < DP_SPLCONN_PKTDEQ_RING) || \
    (DP_MAX_PKTDEQ_RING < DP_NUM_PKTDEQ_RING)
#error "wrong DP_MAX_PKTDEQ_RING value"
#endif

/* this macro should be >= max(DP_NUM_DC_BUFRET_RING, DP_SPLCONN_BUFRET_RING,
 * DP_NUM_BUFRET_RING)
 */
#define DP_MAX_BUFRET_RING 2
#if (DP_MAX_BUFRET_RING < DP_NUM_DC_BUFRET_RING)  || \
    (DP_MAX_BUFRET_RING < DP_SPLCONN_BUFRET_RING) || \
    (DP_MAX_BUFRET_RING < DP_NUM_BUFRET_RING)
#error "wrong DP_MAX_BUFRET value"
#endif

/* this macro should be >= max(DP_NUM_DC_BUFRET_RING, DP_SPLCONN_BUFRET_RING) */
#define DP_MAX_BUFREQ_RING 2
#if (DP_MAX_BUFREQ_RING < DP_NUM_DC_BUFREQ_RING) || \
    (DP_MAX_BUFREQ_RING < DP_SPLCONN_BUFREQ_RING)
#error "wrong DP_MAX_BUFREQ value"
#endif

struct dp_port_info {
	enum PORT_FLAG status;	/*! port status */
	struct dp_cb cb;	/*! Callback Pointer to DIRECTPATH_CB */
	struct module *owner;
	struct net_device *dev;
	int inst; /* instance id */
	struct inst_property *inst_prop; /* instance property pointer */
	u32 dev_port;
	u32 num_subif; /* number of subif registered under this dp_port */
	u16 max_subif;

	/*!< --------------------------------------------------------------
	 *!< start of placeholder for fast copy port_info.
	 *   Here all fileds must be same as defined in &struct dp_subif
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

	atomic_t tx_err_drop;
	atomic_t rx_err_drop;
	struct gsw_itf *itf_info;  /*point to switch interface configuration */
	u32 vap_offset; /*shift bits to get vap value from subif*/
	u32 vap_mask; /*get final vap after bit shift from subif*/
	u32 subif_offset; /*shift bits to get subif value from ADP desc*/
	u32 subif_mask; /*get final subif after bit shift from ADP desc*/
	u32 flag_other; /*save flag from cqm_dp_port_alloc */
	/* number pktdeq ring */
	u32 num_deq;
	/* number pktenq ring */
	u32 num_enq;
	/* number bufret ring */
	u32 num_ret;
	/* number bufreq ring */
	u32 num_req;
	/* number of tx ippu deq ring*/
	u32 num_tx_ippu;
	/* number of rx ippu deq ring*/
	u32 num_rx_ippu;
	/* pktdeq ring */
	struct dp_ring_pktdeq *deq[DP_MAX_PKTDEQ_RING];
	/* pktenq ring, inside of this has 'rx ippu' */
	struct dp_ring_pktenq *enq[DP_MAX_PKTENQ_RING];;
	/* bufreq ring */
	struct dp_ring_bufreq *req[DP_MAX_BUFREQ_RING];
	/* burret ring */
	struct dp_ring_bufret *ret[DP_MAX_BUFRET_RING];
	/* Tx ippu ring*/
	struct dp_ring_pktdeq *tx_ippu[DP_MAX_INTERM];
	/* Rx ippu ring*/
	struct dp_ring_pktdeq *rx_ippu[DP_MAX_INTERM];

	u8 umt_enable; /*Store umt enable flag given by user at dp_register_dev_ext()*/
	/* For CPU: alloc in dp_platform_set and config in dp_platform_set
	 *          Note: for CPU case, gpid[] is for CPU subif only, not store
	 *                spl_conn's GPID at all.
	 * For peripheral device: alloc in dev_platform_set via gpid_port_assign,
	 *                        and config it at dp_register_subif
	 */
	u8 gpid[MAX_GPID_PER_PORT];

	/* reserved maximum nubmer of continuous gpid.
	 * For LPID zero, num_gpid does not includes those gpid for spl_conn
	 */
	u16 num_gpid;
	/* 1st vap to map to 1st GPID
	 * For CPU port, it is first subif/vap used for CPU, currently it is 12.
	 * For non-CPU port, it is always 0
	 */
	u16 vap_offset_gpid;

	u32 res_qid_base; /* Base entry for the device's reserved Q */
	u32 num_resv_q; /* Num of reserved Q per device */
	u32 f_ptp: 1; /* PTP1588 support enablement */
	/*only valid for 1st dp instanace which need dp_xmit/dp_rx*/
	/*[0] for non-checksum case,
	 *[1] for checksum offload
	 *[2] two cases:
	 * a: only traffic directly to MPE DL FW
	 * b: DSL bonding FCS case
	 */
	struct cqm_lookup_sel lookup_sel; /* Save dp_dev_data->lookup_sel
					   * to here if DP_F_DEV_LOOKUP_SEL present
					   * else save DPM insternal lookup sel
					   */
	u32 desc_dw_templ[TEMPL_MAX][4];
	u32 desc_dw_mask[TEMPL_MAX][4];
	u32 blk_size;	/*!< PCE Block Size */
	int qid_base; /*!< current it is only for EPON case */
#ifdef CONFIG_RFS_ACCEL
	struct cpu_rmap  *rx_cpu_rmap; /*!<CPU Affinity Reverse Map for CQM IRQs */
#endif /* CONFIG_RFS_ACCEL */
	u32 data_flag_ops; /* flag_ops from caller */
	struct dp_qos_setting *dts_qos; /* dts_qos for the deq to device */

	/* These members must be end. */
	u32 tail;
	struct dp_subif_info *subif_info;
	spinlock_t mib_cnt_lock; /* lock for updates from mib_counters module*/
};

struct ctp_dev {
	struct list_head list;
	struct net_device *dev; /* CTP device pointer */
	u16 ctp; /* CTP port */
};

/*queue struct */
struct q_info {
	int flag;  /*0-FREE, 1-Used*/
	int need_free; /*if this queue is allocated by dp_register_subif,
			*   it needs free during de-register.
			*Otherwise, no free
			*/
	int q_node_id;
	int ref_cnt; /*subif_counter*/
	int cqm_dequeue_ring; /*CQM dequeue ring or pp QOS port */
};

/*scheduler struct */
struct dp_sched_info {
	int flag;  /*0-FREE, 1-Used*/
	int ref_cnt; /*subif_counter*/
	int cqm_dequeue_ring; /*CQM dequeue ring or pp QOS port */
};

enum CQM_RING_TYPE {
	CQM_DEQ_RING_NORM    = BIT(0),	/* packet dequeue ring to device directly */
	CQM_DEQ_RING_TX_IPPU = BIT(1),	/* packet dequeue ring to device via IPPU/PP */
	CQM_DEQ_RING_RX_IPPU = BIT(2),	/* packet dequeue ring from device to IPPU/PP */
	CQM_DEQ_RING_IPPU    = CQM_DEQ_RING_TX_IPPU | CQM_DEQ_RING_RX_IPPU,
	CQM_ENQ_RING         = BIT(3),	/* packet enqueue ring */
	CQM_RET_RING         = BIT(4), 	/* Buffer return ring */
	CQM_REQ_RING         = BIT(5),	/* Buffer request ring */
};

/* cqm enq ring */
struct cqm_enq_ring_info {
	int ring_id;
	/* Below all are arrays because if IGP is shared resource, need to store
	 * all ports, like wave700
	 */
	uint8_t type; /* bit definition refer to enum CQM_RING_TYPE */
	u8 dp_port[MAX_DP_PORTS]; /* If igp is shared across dp ports, each idx denotes a dp port,
				   * if dp_port[i] >= 0 then i'th dp_port has
				   * this IGP and dp_port[i] is ref_count for that.
				   */

	struct dp_ring_pktenq enq; /*enq ref to
				     *dp_port_info->enq[enq_ring_index]
				     */
	u8 ref_cnt;
};

struct cqm_ret_ring_info {
	int ring_id;
	uint8_t type; /* bit definition refer to enum CQM_RING_TYPE */
	u8 ref_cnt;
	u8 dp_port[MAX_DP_PORTS]; /* If this is shared across dp ports, each idx denotes a dp port,
				   * if dp_port[i] >= 0 then i'th dp_port has
				   */

	struct dp_ring_bufret ret; /*ret ref to
				     *dp_port_info->ret[ret_ring_index]
				     */
};

struct cqm_req_ring_info {
	int ring_id;
	uint8_t type; /* bit definition refer to enum CQM_RING_TYPE */
	u8 ref_cnt;
	u8 dp_port[MAX_DP_PORTS]; /* If this is shared across dp ports, each idx denotes a dp port,
				   * if dp_port[i] >= 0 then i'th dp_port has
				   */

	struct dp_ring_bufreq req; /*req ref to
				     *dp_port_info->req[req_ring_index]
				     */
};

struct cqm_deq_ring_info {
	int ring_id;
	uint8_t type; /* bit definition refer to enum CQM_RING_TYPE */
	u32 ref_cnt; /* reference counter: the number of subif attached to it
		      * which was increased/decreased during dp_register_subif
		      * or shared by different DC RXOUT rings during
		      * dp_register_dev.
		      * For each cqm port, only one mode will be used, each by
		      * CTP or Rxout ring. Never will be both.
		      * Note: these two mode has to share same variable ref_cnt
		      * since alloc_q related API heavily depends on ref_cnt
		      * during dp_register_dev and dp_deregister_dev
		      */
	int qid;
	int q_node; /* first_qid's logical node id*/
	struct dp_ring_pktdeq deq; /*deq ref to
				     *dp_port_info->deq[deq_ring_index]
				     */
	u8 dp_port[MAX_DP_PORTS]; /* If egp is shared across dp ports, each idx denotes a dp port,
				   * if dp_port[i] = 1 then i'th dp_port has
				   * this EGP.
				   */
	struct dp_qos_setting *dts_qos;
};

struct parser_info {
	u8 v;
	s8 size;
};

struct subif_platform_data {
	struct net_device *dev;
	struct dp_subif_data *subif_data;  /*from dp_register_subif_ex */
#define TRIGGER_CQE_DP_ENABLE  1
	int act; /*Set by HAL subif_platform_set and used by DP lib */
};

/*port 0 is reserved*/
extern int dp_inst_num;
extern int dp_print_len;
extern struct inst_property dp_port_prop[DP_MAX_INST];
extern struct dp_port_info *dp_port_info[DP_MAX_INST];
extern struct q_info dp_q_tbl[DP_MAX_INST][DP_MAX_QUEUE_NUM];
extern struct dp_sched_info dp_sched_tbl[DP_MAX_INST][DP_MAX_NODES];
extern struct cqm_deq_ring_info dp_deq_ring_tbl[DP_MAX_INST][DP_MAX_PPV4_PORT];
extern struct cqm_enq_ring_info dp_enq_ring_tbl[DP_MAX_INST][DP_MAX_CQM_ENQ];
extern struct cqm_ret_ring_info dp_ret_ring_tbl[DP_MAX_INST][DP_MAX_CQM_RET];
extern struct cqm_req_ring_info dp_req_ring_tbl[DP_MAX_INST][DP_MAX_CQM_REQ];
extern struct cqm_ops *dp_cqm_ops[DP_MAX_INST];

void dp_die(const char *func_name, int curr_v, int ref_v);

static inline struct inst_property *get_dp_port_prop(int inst)
{
	if ((inst < 0) || (inst >= DP_MAX_INST)) {
		pr_err("DPM: %s wrong inst=%d\n", __func__, inst);
		dp_die(__func__, inst, DP_MAX_INST);
		return &dp_port_prop[0];
	}
	return &dp_port_prop[inst];
}

static inline struct dp_port_info *get_dp_port_info(int inst, int index)
{
	if ((inst < 0) || (inst  >= DP_MAX_INST)) {
		pr_err("DPM: %s wrong inst=%d\n", __func__, inst);
		dp_die(__func__, inst, DP_MAX_INST);
		return &dp_port_info[0][0];
	}
	if ((index < 0) ||
	    (index  >= dp_port_prop[inst].info.cap.max_num_dp_ports)) {
		pr_err("DPM: %s wrong dp_port=%d\n", __func__, index);
		dp_die(__func__, index,
		       dp_port_prop[inst].info.cap.max_num_dp_ports);
		return &dp_port_info[0][0];
	}
	return &dp_port_info[inst][index];
}

static inline struct cqm_deq_ring_info *get_dp_deqring_info(int inst, int idx)
{
	if ((inst < 0) || (inst  >= DP_MAX_INST)) {
		pr_err("DPM: %s wrong inst=%d\n", __func__, inst);
		dp_die(__func__, inst, DP_MAX_INST);
		return &dp_deq_ring_tbl[0][0];
	}
	if ((idx < 0) || (idx  >= DP_MAX_PPV4_PORT)) {
		pr_err("DPM: %s wrong deq_ring=%d\n", __func__, idx);
		dp_die(__func__, idx, DP_MAX_PPV4_PORT);
		return &dp_deq_ring_tbl[0][0];
	}
	return &dp_deq_ring_tbl[inst][idx];
}

static inline struct cqm_enq_ring_info *get_dp_enqring_info(int inst, int idx)
{
	if ((inst < 0) || (inst  >= DP_MAX_INST)) {
		pr_err("DPM: %s wrong inst=%d\n", __func__, inst);
		dp_die(__func__, inst, DP_MAX_INST);
		return &dp_enq_ring_tbl[0][0];
	}
	if ((idx < 0) || (idx  >= DP_MAX_CQM_ENQ)) {
		pr_err("DPM: %s wrong enq_ring=%d\n", __func__, idx);
		dp_die(__func__, idx, DP_MAX_CQM_ENQ);
		return &dp_enq_ring_tbl[0][0];
	}
	return &dp_enq_ring_tbl[inst][idx];
}

static inline struct cqm_ret_ring_info *get_dp_retring_info(int inst, int idx)
{
	if ((inst < 0) || (inst  >= DP_MAX_INST)) {
		pr_err("DPM: %s wrong inst=%d\n", __func__, inst);
		dp_die(__func__, inst, DP_MAX_INST);
		return &dp_ret_ring_tbl[0][0];
	}
	if ((idx < 0) || (idx  >= DP_MAX_CQM_RET)) {
		pr_err("DPM: %s wrong ret_ring=%d\n", __func__, idx);
		dp_die(__func__, idx, DP_MAX_CQM_RET);
		return &dp_ret_ring_tbl[0][0];
	}
	return &dp_ret_ring_tbl[inst][idx];
}

static inline struct cqm_req_ring_info *get_dp_reqring_info(int inst, int idx)
{
	if ((inst < 0) || (inst  >= DP_MAX_INST)) {
		pr_err("DPM: %s wrong inst=%d\n", __func__, inst);
		dp_die(__func__, inst, DP_MAX_INST);
		return &dp_req_ring_tbl[0][0];
	}
	if ((idx < 0) || (idx  >= DP_MAX_CQM_RET)) {
		pr_err("DPM: %s wrong req_ring=%d\n", __func__, idx);
		dp_die(__func__, idx, DP_MAX_CQM_RET);
		return &dp_req_ring_tbl[0][0];
	}
	return &dp_req_ring_tbl[inst][idx];
}

static inline struct q_info *get_dp_q_info(int inst, int idx)
{
	if ((inst < 0) || (inst  >= DP_MAX_INST)) {
		pr_err("DPM: %s wrong inst=%d\n", __func__, inst);
		dp_die(__func__, inst, DP_MAX_INST);
		return &dp_q_tbl[0][0];
	}
	if ((idx < 0) ||
	    (idx  >= dp_port_prop[inst].info.cap.max_num_queues)) {
		pr_err("DPM: %s wrong queue_id=%d\n", __func__, idx);
		dp_die(__func__, idx,
		      dp_port_prop[inst].info.cap.max_num_queues);
		return &dp_q_tbl[0][0];
	}

	return &dp_q_tbl[inst][idx];
}

static inline struct inst_info *get_dp_prop_info(int inst)
{
	if ((inst < 0) || (inst  >= DP_MAX_INST)) {
		pr_err("DPM: %s wrong inst=%d\n", __func__, inst);
		dp_die(__func__, inst, DP_MAX_INST);
		return &dp_port_prop[0].info;
	}
	return &dp_port_prop[inst].info;
}

static inline struct dp_subif_info *get_dp_port_subif(
	const struct dp_port_info *port, u16 vap)
{
	/* Note: here we canot do accurate vap sanity check
	 *       We need call this API to initialize dp_port_info[][].subif_info
	 *       internal list related memory. At that time, subif_info content
	 *       not set yet
	 */
	if (vap >= dp_port_prop[port->inst].info.cap.max_num_subif) {
		pr_err("DPM: %s wrong vap=%u max_subif=%d dp_port=%d\n",
		       __func__, vap,
		       dp_port_prop[port->inst].info.cap.max_num_subif,
		       port->port_id);
		dp_die(__func__, vap, port->max_subif);
	}
	return &port->subif_info[vap];
}

static inline struct dev_mib *get_dp_port_subif_mib(struct dp_subif_info *sif)
{
	return &sif->mib;
}

static inline bool is_soc_tpz(int inst)
{
	return true;
}

/*!
 *@brief is_spl_conn
 *@param[in] lpid    : port LPID
 *@param[in] vap     : VAP id
 *@retrurn bool      : true if CPU Special connection
 */
static inline bool is_spl_conn(int lpid, int vap)
{
	return (lpid == DP_CPU_LPID && vap < (DP_MAX_DPID0_SUBIF - DP_MAX_CPU));
}

static inline bool is_cpu_vap(int lpid, int vap)
{
	return (lpid == DP_CPU_LPID && vap >= DP_DPID0_CPU_SUBIF_START && vap <= DP_DPID0_CPU_SUBIF_END);
}

/*Just find the first valid dp_port from the given array*/
static inline int dp_deq_find_a_dpport(u8 *dp_arr)
{
	int i;
	for (i = 0; i < MAX_DP_PORTS; i++)
		if(dp_arr[i])
			return i;
	return -1;
}

void dp_loop_eth_dev_exit(void);
char *dp_qos_flag_to_str(enum QOS_FLAG qf);
char *dp_arbi_to_str(enum dp_arbitate ar);

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DEBUGFS)
struct dentry *dp_proc_install(void);
#else
static inline struct dentry *dp_proc_install(void) { return NULL;}
#endif

extern char *dp_dbg_flag_str[];
extern u64 dp_dbg_flag_list[];
extern char *dp_dbgfs_flag_str[];
extern u64 dp_dbgfs_flag_list[];
extern u32 dp_port_flag[];
extern char *dp_port_type_str[];
extern char *dp_port_status_str[];
extern struct parser_info pinfo[];

enum TEST_MODE {
	DP_RX_MODE_NORMAL = 0,
	DP_RX_MODE_LAN_WAN_BRIDGE,
	DPR_RX_MODE_MAX
};

extern struct platform_device *g_dp_dev;
extern u32 rx_desc_mask[4];
extern u32 tx_desc_mask[4];
extern const bool dp_dsable_optimize;

ssize_t proc_print_mode_write(struct file *file, const char *buf,
			      size_t count, loff_t *ppos);
void proc_print_mode_read(struct seq_file *s);
int parser_size_via_index(u8 index);
struct dp_port_info *get_port_info_via_dev(struct net_device *dev);
void dp_clear_mib(dp_subif_t *subif, uint32_t flag);
extern u32 dp_drop_all_tcp_err;
extern u32 dp_pkt_size_check;
void print_parser_status(struct seq_file *s);
void proc_mib_timer_read(struct seq_file *s);
int mpe_fh_netfiler_install(void);
#ifdef CONFIG_LTQ_DATAPATH_CPUFREQ
int dp_cpufreq_notify_init(int inst);
int dp_cpufreq_notify_exit(void);
#endif
int proc_qos_init(void *param);
int proc_qos_dump(struct seq_file *s, int pos);
int proc_sched_hal_dump(struct seq_file *s, int pos);
int proc_sched_child_hal_dump(struct seq_file *s, int pos);
ssize_t proc_qos_write(struct file *file, const char *buf,
		       size_t count, loff_t *ppos);

//int dp_reset_sys_mib(u32 flag);
void dp_clear_all_mib_inside(uint32_t flag);

extern int ip_offset_hw_adjust;
struct net_device *get_base_dev(struct net_device *dev, int level);
int dp_inst_init(u32 flag);
void dp_inst_free(void);
int request_dp(u32 flag);
int dp_late_init_module(void);
void dp_cleanup_module(void);
int dp_probe(struct platform_device *pdev);
#define NS_INT16SZ	 2
#define NS_INADDRSZ	 4
#define NS_IN6ADDRSZ	16

int low_10dec(u64 x);
int high_10dec(u64 x);
int dp_atoi(unsigned char *str);
u64 dp_atoull(unsigned char *str);
int get_offset_clear_chksum(struct sk_buff *skb, u32 *ip_offset,
			    u32 *tcp_h_offset,
			    u32 *tcp_type);
int dp_basic_proc(void);

struct dp_port_info *get_port_info_via_dp_port(int inst, int dp_port);

void set_dp_dbg_flag(uint64_t flags);
uint64_t get_dp_dbg_flag(void);
void dp_dump_raw_data(const void *buf, int len, char *prefix_str);
char *dp_skb_csum_str(struct sk_buff *skb);
extern struct dentry *dp_proc_node;
int get_dp_dbg_flag_str_size(void);
int get_dp_dbgfs_flag_str_size(void);
int get_dp_port_status_str_size(void);

int dp_request_inst(struct dp_inst_info *info, u32 flag);
int register_dp_cap(u32 flag);
extern int dp_init_ok;
extern u32 dp_init_state;
extern int dp_cpu_init_ok;

#if IS_ENABLED(CONFIG_QOS_MGR)
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)
extern int32_t (*qos_mgr_hook_setup_tc)(struct net_device *dev, u32 handle,
					__be16 protocol,
					struct tc_to_netdev *tc);
#else
extern int32_t (*qos_mgr_hook_setup_tc)(struct net_device *dev,
					enum tc_setup_type type,
					void *type_data);
#endif
#endif

#define DP_SUBIF_LIST_HASH_SHIFT 8
#define DP_SUBIF_LIST_HASH_BIT_LENGTH 10
#define DP_SUBIF_LIST_HASH_SIZE ((1 << DP_SUBIF_LIST_HASH_BIT_LENGTH) - 1)

int dp_get_drv_mib(dp_subif_t *subif, dp_drv_mib_t *mib, uint32_t flag);
extern struct hlist_head dp_subif_list[DP_SUBIF_LIST_HASH_SIZE];
int32_t dp_sync_subifid(struct net_device *dev, char *subif_name,
			dp_subif_t *subif_id, struct dp_subif_data *data,
			u32 flags);
int32_t dp_sync_subifid_priv(struct net_device *dev, char *subif_name,
			     dp_subif_t *subif_id, struct dp_subif_data *data,
			     u32 flags, dp_get_netif_subifid_fn_t subifid_fn,
			     int f_notif);
int32_t	dp_update_subif(struct net_device *netif, void *data, dp_subif_t *subif,
			char *subif_name, u32 flags,
			dp_get_netif_subifid_fn_t subifid_fn);
int32_t	dp_del_subif(struct net_device *netif, void *data, dp_subif_t *subif,
		     char *subif_name, u32 flags);
struct dp_subif_cache *dp_subif_lookup_safe(
	struct hlist_head *head,
	const struct net_device *dev,
	void *data);
int dp_subif_list_init(void);
void dp_subif_list_free(void);
int dp_lan_wan_bridging(int port_id, struct sk_buff *skb);
u32 alloc_dp_port_subif_info(int inst);
void free_dp_port_subif_info(int inst);
u32 dp_subif_hash(struct net_device *dev);
int dp_cqm_deq_port_enable(struct module *owner, int inst, int port_id,
			   int deq_port_idx, int num_deq_port, int flags,
			   u32 dma_ch_off);
int32_t dp_get_subifid_for_update(int inst, struct net_device *netif,
				  dp_subif_t *subif, uint32_t flags);
int do_tx_hwtstamp(int inst, int dpid, struct sk_buff *skb);
struct dp_evt_notif_info {
	struct notifier_block nb;
	struct dp_event evt_info;
	struct list_head list;
};

struct dp_evt_notif_data {
	enum DP_EVENT_OWNER owner;
	struct net_device *dev;
	struct module *mod;
	u32 dev_port;
	s32 subif;
	int dpid;
	int inst;
	int alloc_flag;
	union {
		struct dp_dev_data *dev_data; /*!< valid only
					       * for DP_EVENT_REGISTER_DEV
					       * event
					       */
	} data;
};

extern struct blocking_notifier_head dp_evt_notif_list;
int register_dp_event_notifier(struct dp_event *info);
int unregister_dp_event_notifier(struct dp_event *info);
void dp_set_tmp_inst(int);

#define dp_dump_debugfs(function) do {\
	int pos = 0; \
	do { \
		dp_set_tmp_inst(0); \
		pos = function(NULL, pos); \
	} while (pos >= 0); \
} while (0)

#if IS_ENABLED(CONFIG_DPM_DATAPATH_DEBUGFS) && IS_ENABLED(CONFIG_DPM_DATAPATH_DBG)
#define dp_dump_debugfs_all(port, subif_grp) do {\
	if (dp_dbgfs_flag & DP_DBGFS_FLAG_DPID) { \
		pr_info("\n------------> Dumping dp/dpid <-------------\n"); \
		dp_dump_debugfs(proc_dpid_dump); \
	} \
	if (dp_dbgfs_flag & DP_DBGFS_FLAG_PORT) { \
		pr_info("\n------------> Dumping dp/port portid: %d, subif_grp: %d <-------------\n", port, subif_grp); \
		dp_set_tmp_inst(0); \
		proc_port_dump_one(NULL, port); \
		pr_info("    Curr Subif:%d info:\n", subif_grp); \
		proc_subif_dump_one(NULL, port, subif_grp); \
	} \
	if (dp_dbgfs_flag & DP_DBGFS_FLAG_DEQ) { \
		pr_info("\n------------> Dumping dp/deq <-------------\n"); \
		dp_dump_debugfs(proc_registration_deq_port_dump); \
	} \
	if (dp_dbgfs_flag & DP_DBGFS_FLAG_QOS) { \
		pr_info("\n------------> Dumping dp/qos <-------------\n"); \
		dp_dump_debugfs(proc_qos_dump); \
	} \
	if (dp_dbgfs_flag & DP_DBGFS_FLAG_ENQ) { \
		pr_info("\n------------> Dumping dp/enq <-------------\n"); \
		dp_dump_debugfs(proc_registration_enq_port_dump); \
	} \
	if (dp_dbgfs_flag & DP_DBGFS_FLAG_DEV) { \
		pr_info("\n------------> Dumping dp/dev <-------------\n"); \
		dp_dump_debugfs(proc_dev_dump); \
	} \
	if (dp_dbgfs_flag & DP_DBGFS_FLAG_DEVOPS) { \
		pr_info("\n------------> Dumping dp/dev_ops <-------------\n"); \
		dp_dump_debugfs(proc_dev_ops_dump); \
	} \
	if (dp_dbgfs_flag & DP_DBGFS_FLAG_HOOKS) { \
		pr_info("\n------------> Dumping dp/hooks <-------------\n"); \
		proc_dp_active_tx_hook_dump(NULL); \
		proc_dp_active_rx_hook_dump(NULL); \
	} \
} while (0)
#define DP_DUMP_DEBUGFS_QOS_ALL(...) do {\
	dp_dump_debugfs(proc_qos_dump); \
	dp_dump_debugfs(proc_sched_hal_dump); \
	dp_dump_debugfs(proc_sched_child_hal_dump); \
} while (0)
#else
#define dp_dump_debugfs_all(...)
#define DP_DUMP_DEBUGFS_QOS_ALL(...)
#endif

static inline bool is_directpath(struct dp_port_info *port)
{
	return (port->alloc_flags & DP_F_DIRECT);
}

static inline bool is_dsl(struct dp_port_info *port)
{
	return (port->alloc_flags & DP_F_FAST_DSL);
}

void dp_print_err_info(int res);
int dp_notifier_invoke(int inst,
		       struct net_device *dev, u32 port_id, u32 subif_id,
		       void *evt_data, enum DP_EVENT_TYPE type);
int proc_dp_event_list_dump(struct seq_file *s, int pos);

int dp_dealloc_cqm_port(struct module *owner, u32 dev_port,
			struct dp_port_info *port,
			struct cqm_dp_alloc_data *data, u32 flags);

int dp_alloc_cqm_port(struct module *owner, struct net_device *dev,
		      u32 dev_port, s32 port_id,
		      struct cqm_dp_alloc_data *cqm_data, u32 flags);

int dp_enable_cqm_rings(struct module *owner, struct dp_port_info *port,
		       struct cqm_dp_en_data *data, u32 flags);

int dp_cqm_port_alloc_complete(struct module *owner,
			       struct dp_port_info *port, s32 dp_port,
			       struct cqm_dp_alloc_complete_data *data,
			       u32 flags);
int dp_cqm_gpid_lpid_map(int inst, struct cqm_gpid_lpid *map);
int dp_cqm_queue_map_set(int inst, s32 qid, cqm_queue_map_entry_t *entry, u32 flags);
int dp_cqm_queue_map_get(int inst, s32 qid, s32 *num_entry, uint16_t **entry, u32 flags);
int dp_cqm_queue_map_buf_free(int inst, uint16_t *qmap_entry);
int dp_cqm_spl_conn(int inst, struct dp_spl_cfg_priv *conn);
int dp_cqm_get_policy_map(int inst, int base, int range, u32 alloc_flags, int flags);
int dp_cqm_get_dflt_resv(int inst, struct cqm_dflt_resv *resv, int flags);
int dp_cqm_get_mtu_size(int inst, struct cqm_mtu *mtu);
int dp_cqm_qsv_get(int inst, struct cqm_lookup_sel *entry, int flags);
int dp_cqm_qsv_set(int inst, struct cqm_lookup_sel *entry, int flags);
int dp_cqm_buffer_free_by_policy(int inst, struct cqm_bm_free *data);
int dp_cqm_get_dc_config(int inst, struct cqm_dc_res *r, int flag);
int dp_cqm_cpu_port_get(int inst, struct cqm_cpu_port_data *cpu_data, int flags);
int dp_cqm_get_lookup_qid_via_index(int inst, struct cqm_lookup *info);
int dp_cqm_map_to_drop_q(int inst, struct cqm_lookup_entry *lu);
int dp_cqm_qos_queue_flush(int inst, struct dp_ring_index index, int qid, u32 node_id);
int dp_cqm_restore_orig_q(int inst, struct cqm_lookup_entry *lu);
int dp_cqm_qid2ep_map_set(int inst, int qid, int cqm_deq_ring);
int free_cqm_cpu_port_data(struct cqm_cpu_port_data *cpu_data);
int alloc_cqm_cpu_port_data(struct cqm_cpu_port_data *cpu_data);

int dp_manual_init_exit_trigger(bool exit_flag);
//int dp_init_pce(void);
int proc_dpid_dump(struct seq_file *s, int pos);
int proc_port_dump_one(struct seq_file *s, int pos);
char *dp_get_sym_name_by_addr(void *symaddr, char *symname,
		const char *fail_str);
int proc_subif_dump_one(struct seq_file *s, int pos, int subif);
int proc_registration_deq_port_dump(struct seq_file *s, int pos);
int proc_registration_enq_port_dump(struct seq_file *s, int pos);
//int proc_pce_dump(struct seq_file *s, int pos);
//ssize_t proc_pce_write(struct file *file, const char *buf, size_t count,
//		       loff_t *ppos);
void proc_dp_active_tx_hook_dump(struct seq_file *);
void proc_dp_active_rx_hook_dump(struct seq_file *);
int proc_dev_ops_dump(struct seq_file *s, int pos);
int proc_dev_dump(struct seq_file *s, int pos);
int dp_ctp_dev_list_add(struct list_head *head, struct net_device *dev,
			int vap);
int dp_ctp_dev_list_del(struct list_head *head, struct net_device *dev);
int dp_update_shared_bp_to_subif(int inst, struct net_device *netif, int bp,
				 int portid);
void dump_cpu_data(int inst, struct cqm_cpu_port_data *p);
int dp_free_deq_ring(int inst, u8 ep, struct dp_dev_data *data,
			uint32_t flags);
int _dp_init_subif_q_map_rules(struct dp_subif_info *subif_info,
					    int spl_conn_igp);
int _dp_set_subif_q_lookup_tbl(struct dp_subif_info *subif_info,
			       int cls_idx,
			       /* spl_conn_igp_qid must be set if this
				* API call is for sp_conn queue mapping
				*/
			       int spl_conn_igp_qid);
int _dp_reset_subif_q_lookup_tbl(struct dp_subif_info *subif_info,
				 int cls_idx);
int _dp_reset_q_lookup_tbl(int inst);
//bool subif_bit8_workaround(struct dp_subif_info *subif_info);
u32 get_subif_q_map_reset_flag(int inst);
int dp_add_subif_spl_dev(int inst,
				   struct net_device *dev,
				   char *subif_name,
				   dp_subif_t *subif_id,
				   struct dp_subif_data *data,
				   u32 flags);
int dp_del_subif_spl_dev(int inst,
				   struct net_device *dev,
				   char *subif_name,
				   dp_subif_t *subif_id,
				   struct dp_subif_data *data,
				   u32 flags);

int dp_switchdev_register_notifiers(void);
void dp_switchdev_unregister_notifiers(void);
bool dp_valid_netif(const struct net_device *netif);
void dp_free_remaining_dev_list(void);
char *dp_strsep(char **stringp, const char *delim);
void dp_dump_addr(struct seq_file *s);
void dp_gdb_break(void);
int dp_set_cpu_mac(struct net_device *dev, bool reset);
int dp_datapath_dts_parse(void);
int proc_qos_cfg_dump(struct seq_file *s, int pos);
#if IS_ENABLED(CONFIG_OF)
void proc_qos_raw_dts_dump(struct seq_file *s);
#endif
void proc_qos_category_dump(struct seq_file *s);
int alloc_flag_str(int flag, char *buf, int buf_len);
ssize_t proc_dts_raw_write(struct file *file, const char *buf, size_t count,
			   loff_t *ppos);
void init_qos_setting(void);
struct dp_qos_setting* dp_get_qos_cfg(int inst, int dp_port, int alloc_flag, u32 qos_id);
struct dp_qos_setting* dp_get_inter_qos_cfg(int inst, u32 deq_port);

static inline bool is_xpon(int flags)
{
	if (flags & (DP_F_GPON))
		return true;

	return false;
}

/* These below apis used to seemless switch b/w seqfs and pr_info */
#define dp_sprintf(seq, fmt, ...) do {\
	if (seq) \
		seq_printf(seq, fmt, ##__VA_ARGS__); \
	else \
		pr_cont(fmt, ##__VA_ARGS__); \
} while (0)

static inline bool dp_seq_has_overflowed(struct seq_file *seq)
{
	bool ret = false;
	if (seq)
		ret = seq_has_overflowed(seq);
	return ret;
}

#define dp_sputc(seq, c) do {\
	if (seq) \
		seq_putc(seq, c); \
	else	\
		pr_cont("%c", c); \
} while (0)

#define dp_sputs(seq, str) do {\
	if (seq) \
		seq_puts(seq, str); \
	else \
		pr_cont("%s", str);	\
} while (0)

void __dp_dump_pktdeq_ring(int indent, struct dp_ring_pktdeq *r, int n, bool umt_en, bool ippu, bool tx);
#define dp_dump_pktdeq_ring(...) __dp_dump_pktdeq_ring(__VA_ARGS__, false, false)
#define dp_dump_tx_ippu_ring(...) __dp_dump_pktdeq_ring(__VA_ARGS__, false, true, true)
#define dp_dump_rx_ippu_ring(...) __dp_dump_pktdeq_ring(__VA_ARGS__, false, true, false)
void dp_dump_dp_dev_data(int, struct dp_dev_data *);
void dp_dump_dp_port_data(int, struct dp_port_data *);
void dp_dump_ring_attr(struct seq_file *s, int indent, struct dp_ring_attr *a, bool umt_en);
int dp_merge_single_mask_bit(cqm_queue_map_entry_t *lookup,
			     struct dp_q_map_sel_pair *tmp);
struct net_device * __init dp_create_netdev(const char *name);
#endif /*DATAPATH_H */
