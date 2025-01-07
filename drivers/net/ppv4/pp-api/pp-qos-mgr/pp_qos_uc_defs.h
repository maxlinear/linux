/*
 * Copyright (C) 2020-2024 MaxLinear, Inc.
 * Copyright (C) 2017-2020 Intel Corporation
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR APARTICULARPURPOSE.See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public
 * License along with this program; if not,see
 * <http://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Description: Packet Processor QoS Driver
 */

#ifndef SRC_UC_HOST_DEFS_H_
#define SRC_UC_HOST_DEFS_H_

/* UC version */
#define UC_VERSION_MAJOR (1)
#define UC_VERSION_MINOR (25)

#define QOS_MAX_PREDECESSORS            (6)
#define QOS_AQM_CONTEXT_MAX_QUEUES      (8)
#define QOS_AQM_MAX_BINS                (16)
#define QOS_ENHANCED_WSP_MAX_QUEUES_PRX (12)
#define QOS_MAX_NUM_QUEUES              (512)
#define AQM_MODE_NORMAL                 (1)
#define AQM_MODE_NO_DROP                (2)

#define WRED_REG_BASE               (0x20000)

/* WRED FLM Registers */
#define WRED_PORT_YELLOW_BYTES_COUNTER(p) (WRED_REG_BASE + 0x200 + 4 * (p))
#define WRED_PORT_GREEN_BYTES_COUNTER(p) (WRED_REG_BASE + 0x400 + 4 * (p))
#define WRED_PORT_YELLOW_THRESHOLD(p) (WRED_REG_BASE + 0x600 + 4 * (p))
#define WRED_PORT_GREEN_THRESHOLD(p) (WRED_REG_BASE + 0x800 + 4 * (p))
#define WRED_QUEUE_TO_PORT_LOOKUP(q) (WRED_REG_BASE + 0xA00 + 4 * (q))
#define WRED_Q_SIZE_BYTES_DIRECT_READ(q) (WRED_REG_BASE + 0xE00 + 4 * (q))

/* WRED LGM Registers */
#define WRED_PORT_YELLOW_BYTES_COUNTER_LGM(p) (WRED_REG_BASE + 0x1000 + 4 * (p))
#define WRED_PORT_GREEN_BYTES_COUNTER_LGM(p) (WRED_REG_BASE + 0x1400 + 4 * (p))
#define WRED_PORT_YELLOW_THRESHOLD_LGM(p) (WRED_REG_BASE + 0x2000 + 4 * (p))
#define WRED_PORT_GREEN_THRESHOLD_LGM(p) (WRED_REG_BASE + 0x2400 + 4 * (p))
#define WRED_QUEUE_TO_PORT_LOOKUP_LGM(q) (WRED_REG_BASE + 0x3000 + 4 * (q))
#define WRED_Q_SIZE_BYTES_DIRECT_READ_LGM(q) (WRED_REG_BASE + 0x4000 + 4 * (q))

/* WRED LGM B-Step Registers */
#define WRED_INIT_REG (WRED_REG_BASE + 0x330)

#define FW_OK_CODE       (0xCAFECAFE)
#define PPV4_QOS_UC_GUARD_MAGIC        FW_OK_CODE

/**************************************************************************
 *! @enum uc_module
 **************************************************************************
 *
 * @brief UC HW modules for logs enum
 *
 **************************************************************************/
enum uc_module {
	UC_MOD_TSCD,
	UC_MOD_TXMNGR,
	UC_MOD_QMNGR,
	UC_MOD_WRED,
	UC_MOD_CNT
};

/* dump writes to all modules registers */
#define UC_MOD_ALL                                                             \
	(BIT_ULL(UC_MOD_TSCD) | BIT_ULL(UC_MOD_TXMNGR) |                       \
	 BIT_ULL(UC_MOD_QMNGR) | BIT_ULL(UC_MOD_WRED))

/**************************************************************************
 *! @enum UC_STATUS
 **************************************************************************
 *
 * @brief UC general status enum
 *
 **************************************************************************/
enum uc_status {
	//!< Status OK
	UC_STATUS_OK,

	//!< General failure
	UC_STATUS_GENERAL_FAILURE,

	//!< Invalid user input
	UC_STATUS_INVALID_INPUT,
};

/**************************************************************************
 *! @enum UC_LOGGER_LEVEL
 **************************************************************************
 *
 * @brief UC Logger level enum. It is recommended to use the defines below
 * for presets
 *
 **************************************************************************/
enum uc_logger_level {
	//!< No log level
	UC_LOGGER_LEVEL_NONE          = 0,

	//!< FATAL error occurred. SW will probably fail to proceed
	UC_LOGGER_LEVEL_FATAL_ONLY    = BIT_ULL(0),

	//!< General ERROR occurred.
	UC_LOGGER_LEVEL_ERROR_ONLY    = BIT_ULL(1),

	//!< WARNING
	UC_LOGGER_LEVEL_WARNING_ONLY  = BIT_ULL(2),

	//!< Information print to the user
	UC_LOGGER_LEVEL_INFO_ONLY     = BIT_ULL(3),

	//!< Debug purposes level
	UC_LOGGER_LEVEL_DEBUG_ONLY    = BIT_ULL(4),

	//!< Dump all writings to registers
	UC_LOGGER_LEVEL_DUMP_REG_ONLY = BIT_ULL(5),

	//!< Dump all commands
	UC_LOGGER_LEVEL_COMMANDS_ONLY = BIT_ULL(6),
};

/* Below levels will be normally used from host. */
/* Each level includes all higher priorities levels messages */

//!< FATAL/ERROR level messages
#define UC_LOGGER_LEVEL_DEFAULT         \
	(UC_LOGGER_LEVEL_FATAL_ONLY |   \
	 UC_LOGGER_LEVEL_ERROR_ONLY)

//!< FATAL level messages
#define UC_LOGGER_LEVEL_FATAL           \
	(UC_LOGGER_LEVEL_FATAL_ONLY |   \
	 UC_LOGGER_LEVEL_COMMANDS_ONLY)

//!< ERRORS level messages
#define UC_LOGGER_LEVEL_ERROR           \
	(UC_LOGGER_LEVEL_FATAL_ONLY |   \
	 UC_LOGGER_LEVEL_ERROR_ONLY |   \
	 UC_LOGGER_LEVEL_COMMANDS_ONLY)

//!< WARNING level messages
#define UC_LOGGER_LEVEL_WARNING         \
	(UC_LOGGER_LEVEL_FATAL_ONLY   | \
	 UC_LOGGER_LEVEL_ERROR_ONLY   | \
	 UC_LOGGER_LEVEL_WARNING_ONLY | \
	 UC_LOGGER_LEVEL_COMMANDS_ONLY)

//!< INFO level messages
#define UC_LOGGER_LEVEL_INFO            \
	(UC_LOGGER_LEVEL_FATAL_ONLY   | \
	 UC_LOGGER_LEVEL_ERROR_ONLY   | \
	 UC_LOGGER_LEVEL_WARNING_ONLY | \
	 UC_LOGGER_LEVEL_INFO_ONLY    | \
	 UC_LOGGER_LEVEL_COMMANDS_ONLY)

//!< DEBUG level messages
#define UC_LOGGER_LEVEL_DEBUG           \
	(UC_LOGGER_LEVEL_FATAL_ONLY   | \
	 UC_LOGGER_LEVEL_ERROR_ONLY   | \
	 UC_LOGGER_LEVEL_WARNING_ONLY | \
	 UC_LOGGER_LEVEL_INFO_ONLY    | \
	 UC_LOGGER_LEVEL_DEBUG_ONLY   | \
	 UC_LOGGER_LEVEL_COMMANDS_ONLY)

//!< DUMP to registers level messages
#define UC_LOGGER_LEVEL_DUMP_REGS        \
	(UC_LOGGER_LEVEL_FATAL_ONLY    | \
	 UC_LOGGER_LEVEL_ERROR_ONLY    | \
	 UC_LOGGER_LEVEL_WARNING_ONLY  | \
	 UC_LOGGER_LEVEL_DUMP_REG_ONLY | \
	 UC_LOGGER_LEVEL_COMMANDS_ONLY)

/**************************************************************************
 *! @enum UC_LOGGER_MODE
 **************************************************************************
 *
 * @brief UC Logger operation mode
 *
 **************************************************************************/
enum uc_logger_mode {
	//!< Logger is disabled
	UC_LOGGER_MODE_NONE,

	//!< Messages are written to the standard output
	UC_LOGGER_MODE_STDOUT,

	//!< Local file. N/A
//	UC_LOGGER_MODE_LOCAL_FILE,

	//!< Messages are written to the host allocated memory
	UC_LOGGER_MODE_WRITE_HOST_MEM,
};

/**************************************************************************
 *! \enum TSCD_NODE_CONF
 **************************************************************************
 *
 * \brief TSCD node configuration valid bits. Used in modify existing node
 *
 **************************************************************************/
enum tscd_node_conf {
	//!< None
	TSCD_NODE_CONF_NONE               = 0,

	//!< Suspend/Resume node
	TSCD_NODE_CONF_SUSPEND_RESUME     = BIT_ULL(0),

	//!< first child (Not relevant for queue)
	TSCD_NODE_CONF_FIRST_CHILD        = BIT_ULL(1),

	//!< last child (Not relevant for queue)
	TSCD_NODE_CONF_LAST_CHILD         = BIT_ULL(2),

	//!< 0 - BW Limit disabled >0 - define BW
	TSCD_NODE_CONF_BW_LIMIT           = BIT_ULL(3),

	//!< Max burst size (number of quantas = 1 << shift)
	TSCD_NODE_CONF_MAX_BURST          = BIT_ULL(4),

	//!< Best Effort enable
	TSCD_NODE_CONF_BEST_EFFORT_ENABLE = BIT_ULL(5),

	//!< First Weighted-Round-Robin node (Not relevant for queue)
	TSCD_NODE_CONF_FIRST_WRR_NODE     = BIT_ULL(6),

	//!< Node Weight (Not relevant for ports)
	TSCD_NODE_CONF_NODE_WEIGHT        = BIT_ULL(7),

	//!< Update predecessor 0 (Not relevant for port)
	TSCD_NODE_CONF_PREDECESSOR_0      = BIT_ULL(8),

	//!< Update predecessor 1 (Not relevant for port)
	TSCD_NODE_CONF_PREDECESSOR_1      = BIT_ULL(9),

	//!< Update predecessor 2 (Not relevant for port)
	TSCD_NODE_CONF_PREDECESSOR_2      = BIT_ULL(10),

	//!< Update predecessor 3 (Not relevant for port)
	TSCD_NODE_CONF_PREDECESSOR_3      = BIT_ULL(11),

	//!< Update predecessor 4 (Not relevant for port)
	TSCD_NODE_CONF_PREDECESSOR_4      = BIT_ULL(12),

	//!< Update predecessor 5 (Not relevant for port)
	TSCD_NODE_CONF_PREDECESSOR_5      = BIT_ULL(13),

	//!< Set if Queue's port was changed (Relevant only for queue)
	TSCD_NODE_CONF_SET_PORT_TO_QUEUE  = BIT_ULL(14),

	//!< Shared BW limit group (0: no shared BW limit, 1-511: group ID)
	TSCD_NODE_CONF_SHARED_BWL_GROUP   = BIT_ULL(15),

	//!< All flags are set
	TSCD_NODE_CONF_ALL                = 0xFFFF
};

/**************************************************************************
 *! \enum QUEUE_CONF
 **************************************************************************
 *
 * \brief queue configuration valid bits. Used in modify existing queue
 *
 **************************************************************************/
enum queue_conf {
	//!< None
	QUEUE_CONF_NONE                     = 0,

	//!< Q is active
	WRED_QUEUE_CONF_ACTIVE_Q            = BIT_ULL(0),

	//!< Disable flags valid
	WRED_QUEUE_CONF_DISABLE             = BIT_ULL(1),

	//!< Use fixed green drop probability
	WRED_QUEUE_CONF_FIXED_GREEN_DROP_P  = BIT_ULL(2),

	//!< Use fixed yellow drop probability
	WRED_QUEUE_CONF_FIXED_YELLOW_DROP_P = BIT_ULL(3),

	//!< Min average yellow
	WRED_QUEUE_CONF_MIN_AVG_YELLOW      = BIT_ULL(4),

	//!< Max average yellow
	WRED_QUEUE_CONF_MAX_AVG_YELLOW      = BIT_ULL(5),

	//!< Slope yellow
	WRED_QUEUE_CONF_SLOPE_YELLOW        = BIT_ULL(6),

	//!< INTERNAL CONFIGURATION. SHOULD NOT BE SET BY HOST
	WRED_QUEUE_CONF_SHIFT_AVG_YELLOW    = BIT_ULL(7),

	//!< Min average green
	WRED_QUEUE_CONF_MIN_AVG_GREEN       = BIT_ULL(8),

	//!< Max average green
	WRED_QUEUE_CONF_MAX_AVG_GREEN       = BIT_ULL(9),

	//!< Slope green
	WRED_QUEUE_CONF_SLOPE_GREEN         = BIT_ULL(10),

	//!< INTERNAL CONFIGURATION. SHOULD NOT BE SET BY HOST
	WRED_QUEUE_CONF_SHIFT_AVG_GREEN     = BIT_ULL(11),

	//!< Min guaranteed
	WRED_QUEUE_CONF_MIN_GUARANTEED      = BIT_ULL(12),

	//!< max allowed
	WRED_QUEUE_CONF_MAX_ALLOWED         = BIT_ULL(13),

	//!< CoDel enable
	TXMGR_QUEUE_CODEL_EN                = BIT_ULL(14),

	//!< Fast Q
	QM_FAST_Q                           = BIT_ULL(15),

	//!< All flags are set
	QUEUE_CONF_ALL                      = 0xFFFF
};

/**************************************************************************
 *! \enum PORT_CONF
 **************************************************************************
 *
 * \brief Port configuration valid bits. Used in modify existing port
 *
 **************************************************************************/
enum port_conf {
	//!< None
	PORT_CONF_NONE              = 0,

	//!< Ring Size
	PORT_CONF_RING_SIZE         = BIT_ULL(0),

	//!< Ring address high
	PORT_CONF_RING_ADDRESS_HIGH = BIT_ULL(1),

	//!< Ring address low
	PORT_CONF_RING_ADDRESS_LOW  = BIT_ULL(2),

	//!< Enable port
	PORT_CONF_ACTIVE            = BIT_ULL(3),

	//!< Green bytes threshold
	PORT_CONF_GREEN_THRESHOLD   = BIT_ULL(4),

	//!< Yellow bytes threshold
	PORT_CONF_YELLOW_THRESHOLD  = BIT_ULL(5),

	//!< All flags are set
	PORT_CONF_ALL               = 0xFFFF
};

#define WRED_AQM_NUM_CONTEXTS                    	(16)

/**************************************************************************
 *! \struct	wred_aqm_conf_t
 **************************************************************************
 *
 * \brief WRED aqm configuration data to set
 *
 **************************************************************************/
typedef struct wred_aqm_conf_s {
	u32	enable;
	u32	peak_rate;
	u32	msr;
	u32	buffer_size;
	u32	msrtokens_addr;
	u32	num_queues;
	u32	queue_id[8];
	double	latency_target;
	u32	llsf;
	u32	coupled_sf;
	u32	coupling_factor;
	u32	amsr;
	u32	msr_l;
	u32	weight; /*Scheduling Weight / 256 */
	u32	num_hist_bins;
	u32	bin_edges[QOS_AQM_MAX_BINS - 1];
} wred_aqm_conf_t;

typedef struct wred_aqm_dbg_s {
	u32 prev_queue_length;
	u32 prev_msrtokens;
	double prev_qdelay;
	double prev_drop_prob;
	double prev_accu_prob;
	u8 prev_burst_allowance;
	u8 prev_ctrl_path_cond;
	u8 prev_qdelay_status;
	u8 prev_burst_state;
	u32 prev_drop_cnt;
	u32 prev_burst_allow_val;
	u32 interrupt_count;
} wred_aqm_dbg_t;

typedef struct wred_aqm_ctx_s {
	wred_aqm_conf_t aqm_conf;
	double drop_prob_;
	double qdelay_old_;
	wred_aqm_dbg_t aqm_dbg;
	u32 old_coupled_queue_length;

	/* Histogram */
	u32    hist_counter[QOS_AQM_MAX_BINS];
	u32    hist_updates;
	u32    max_latency;
} wred_aqm_ctx_t;

typedef struct wred_aqm_db_s {
	wred_aqm_ctx_t aqm_ctx[WRED_AQM_NUM_CONTEXTS];
	u32 aqm_timer;
	u32 wred_aqm_enable_bitmap;
	u32 timer_activation;
} wred_aqm_db_t;

/**************************************************************************
 *! \struct port_stats_s
 **************************************************************************
 *
 * \brief Port stats
 *
 **************************************************************************/
struct port_stats_s {
	u32 total_green_bytes;
	u32 total_yellow_bytes;

	/* Following stats can not be reset */
	u32 debug_back_pressure_status;
	u32 debug_actual_packet_credit;
	u32 debug_actual_byte_credit;
} __attribute__((packed));

/**************************************************************************
 *! \struct hist_stats_s
 **************************************************************************
 *
 * \brief Histogram stats
 *
 **************************************************************************/
struct hist_stats_s {
	u32    num_hist_bins;
	u32    hist_counter[QOS_AQM_MAX_BINS];
	u32    hist_updates;
	u32    max_latency;
} __attribute__((packed));

/**************************************************************************
 *! \struct codel_qstats_s
 **************************************************************************
 *
 * \brief Codel queue stats
 *
 **************************************************************************/
struct codel_qstats_s {
	u32 drop_pkts;
	u32 drop_bytes;
	u32 min_packet_sojourn_time;
	u32 max_packet_sojourn_time;
	u32 total_sojourn_time;
	u32 max_pkt_sz;
	u32 num_pkts;
} __attribute__((packed));

/**************************************************************************
 *! \struct codel_qstate_s
 **************************************************************************
 *
 * \brief Codel queue state
 *
 **************************************************************************/
struct codel_qstate_s {
	u32 count;
	u32 last_count;
	u32 first_above_time;
	u32 drop_next;
	u32 drop_rec_inv_sqrt;
} __attribute__((packed));

/**************************************************************************
 *! \struct hw_node_info_s
 **************************************************************************
 *
 * \brief HW node info
 *
 **************************************************************************/
struct hw_node_info_s {
	u32 first_child;
	u32 last_child;
	u32 is_suspended;
	u32 bw_limit;
	u32 predecessor0;
	u32 predecessor1;
	u32 predecessor2;
	u32 predecessor3;
	u32 predecessor4;
	u32 predecessor5;
	u32 queue_physical_id;
	u32 queue_port;
	u32 weight;
} __attribute__((packed));

/**************************************************************************
 *! \enum PORT_STATS_CLEAR_FLAGS
 **************************************************************************
 *
 * \brief port stats clear flags.
 *        Used in get port stats command to set which stats
 *        will be reset after read
 *
 **************************************************************************/
enum port_stats_clear_flags {
	//!< None
	PORT_STATS_CLEAR_NONE               = 0,

	//!< Clear port total green bytes stats
	PORT_STATS_CLEAR_TOTAL_GREEN_BYTES  = BIT_ULL(0),

	//!< Clear port total yellow bytes stats
	PORT_STATS_CLEAR_TOTAL_YELLOW_BYTES = BIT_ULL(1),

	//!< All above stats will be cleared
	PORT_STATS_CLEAR_ALL                = 0xFFFF,
};

/**************************************************************************
 *! \struct queue_stats_s
 **************************************************************************
 *
 * \brief Queue stats
 *
 **************************************************************************/
struct queue_stats_s {
	/* WRED counters */
	u32 queue_size_bytes;
	u32 queue_average_size_bytes;
	u32 queue_size_entries;
	u32 drop_p_yellow;
	u32 drop_p_green;
	u32 total_bytes_added_low;
	u32 total_bytes_added_high;
	u32 total_accepts;
	u32 total_drops;
	u32 total_dropped_bytes_low;
	u32 total_dropped_bytes_high;
	u32 total_red_dropped;

	/* Following stats can not be reset */
	u32 qmgr_num_queue_entries;
} __attribute__((packed));

struct queue_stat_info {
	u32 qid;
	struct queue_stats_s qstat;
} __attribute__((packed));

#define NUM_Q_ON_QUERY (32U)
struct qm_info {
	u32 num_queues;
	struct queue_stat_info qstat[NUM_Q_ON_QUERY];
} __attribute__((packed));

/**************************************************************************
 *! \enum QUEUE_STATS_CLEAR_FLAGS
 **************************************************************************
 *
 * \brief queue stats clear flags.
 *        Used in get queue stats command to set which stats
 *        will be reset after read
 *
 **************************************************************************/
enum queue_stats_clear_flags {
	//!< None
	QUEUE_STATS_CLEAR_NONE                = 0,

	//!< Clear queue size bytes stats
	QUEUE_STATS_CLEAR_Q_SIZE_BYTES        = BIT_ULL(0),

	//!< Clear queue average size bytes stats
	QUEUE_STATS_CLEAR_Q_AVG_SIZE_BYTES    = BIT_ULL(1),

	//!< Clear queue size entries stats
	QUEUE_STATS_CLEAR_Q_SIZE_ENTRIES      = BIT_ULL(2),

	//!< Clear drop probability yellow stats
	QUEUE_STATS_CLEAR_DROP_P_YELLOW       = BIT_ULL(3),

	//!< Clear drop probability green stats
	QUEUE_STATS_CLEAR_DROP_P_GREEN        = BIT_ULL(4),

	//!< Clear total bytes added stats
	QUEUE_STATS_CLEAR_TOTAL_BYTES_ADDED   = BIT_ULL(5),

	//!< Clear total accepts stats
	QUEUE_STATS_CLEAR_TOTAL_ACCEPTS       = BIT_ULL(6),

	//!< Clear total drops stats
	QUEUE_STATS_CLEAR_TOTAL_DROPS         = BIT_ULL(7),

	//!< Clear total dropped bytes stats
	QUEUE_STATS_CLEAR_TOTAL_DROPPED_BYTES = BIT_ULL(8),

	//!< Clear total RED drops stats
	QUEUE_STATS_CLEAR_TOTAL_RED_DROPS     = BIT_ULL(9),

	//!< All above stats will be cleared
	QUEUE_STATS_CLEAR_ALL                 = 0xFFFF,
};

/**************************************************************************
 *! \struct system_stats_s
 **************************************************************************
 *
 * \brief system stats
 *
 **************************************************************************/
struct system_stats_s {
	u32 qmgr_cache_free_pages_counter;
	u32 qmgr_sm_current_state;
	u32 qmgr_cmd_machine_busy;
	u32 qmgr_cmd_machine_pop_busy;
	u32 qmgr_null_pop_counter;
	u32 qmgr_empty_pop_counter;
	u32 qmgr_null_push_counter;
	u32 qmgr_ddr_stop_push_low_threshold;
	u32 qmgr_fifo_error_register;
	u32 qmgr_ocp_error_register;
	u32 qmgr_cmd_machine_sm_current_state_0;
	u32 qmgr_cmd_machine_sm_current_state_1;
	u32 qmgr_cmd_machine_sm_current_state_2;
	u32 qmgr_cmd_machine_sm_current_state_3;
	u32 qmgr_cmd_machine_sm_current_state_4;
	u32 qmgr_cmd_machine_sm_current_state_5;
	u32 qmgr_cmd_machine_sm_current_state_6;
	u32 qmgr_cmd_machine_sm_current_state_7;
	u32 qmgr_cmd_machine_sm_current_state_8;
	u32 qmgr_cmd_machine_sm_current_state_9;
	u32 qmgr_cmd_machine_sm_current_state_10;
	u32 qmgr_cmd_machine_sm_current_state_11;
	u32 qmgr_cmd_machine_sm_current_state_12;
	u32 qmgr_cmd_machine_sm_current_state_13;
	u32 qmgr_cmd_machine_sm_current_state_14;
	u32 qmgr_cmd_machine_sm_current_state_15;
	u32 qmgr_cmd_machine_queue_0;
	u32 qmgr_cmd_machine_queue_1;
	u32 qmgr_cmd_machine_queue_2;
	u32 qmgr_cmd_machine_queue_3;
	u32 qmgr_cmd_machine_queue_4;
	u32 qmgr_cmd_machine_queue_5;
	u32 qmgr_cmd_machine_queue_6;
	u32 qmgr_cmd_machine_queue_7;
	u32 qmgr_cmd_machine_queue_8;
	u32 qmgr_cmd_machine_queue_9;
	u32 qmgr_cmd_machine_queue_10;
	u32 qmgr_cmd_machine_queue_11;
	u32 qmgr_cmd_machine_queue_12;
	u32 qmgr_cmd_machine_queue_13;
	u32 qmgr_cmd_machine_queue_14;
	u32 qmgr_cmd_machine_queue_15;
	u32 tscd_num_of_used_nodes;

	/* Error in Scheduler tree configuration */
	u32 tscd_infinite_loop_error_occurred;

	/* HW failed to complete the bwl credits updates */
	u32 tscd_bwl_update_error_occurred;

	/* Quanta size in KB */
	u32 tscd_quanta;

	/* WRED Fake pops - LGM B step */
	u32 wred_fake_pops;

	/* WRED pop byte underflow count - LGM B step */
	u32 wred_pop_underflow_count;

	/* WRED pop byte underflow sum - LGM B step */
	u32 wred_pop_underflow_sum;

	/* WRED last push high address - LGM B step */
	u32 wred_last_push_address_high;

	/* WRED last push low address - LGM B step */
	u32 wred_last_push_address_low;

	/* WRED last push drop - LGM B step */
	u32 wred_last_push_drop;

	/* WRED last push info Q id - LGM B step */
	u32 wred_last_push_info_q_id;

	/* WRED last push info color - LGM B step */
	u32 wred_last_push_info_color;

	/* WRED last push info packet size - LGM B step */
	u32 wred_last_push_info_pkt_size;

	/* WRED last pop info Q id - LGM B step */
	u32 wred_last_pop_info_q_id;

	/* WRED last pop info fake - LGM B step */
	u32 wred_last_pop_info_fake;

	/* WRED last pop info color - LGM B step */
	u32 wred_last_pop_info_color;

	/* WRED last pop info packet size - LGM B step */
	u32 wred_last_pop_info_pkt_size;

	/* TX Manager BP status per port words 0-7 */
	u32 txmgr_bp_status_ports_0_31;
	u32 txmgr_bp_status_ports_32_63;
	u32 txmgr_bp_status_ports_64_95;
	u32 txmgr_bp_status_ports_96_127;
	u32 txmgr_bp_status_ports_128_159; /* LGM only */
	u32 txmgr_bp_status_ports_160_191; /* LGM only */
	u32 txmgr_bp_status_ports_192_223; /* LGM only */
	u32 txmgr_bp_status_ports_224_255; /* LGM only */
	u32 dfs_irq_cnt;        /* LGM only */
	u32 dfs_false_irq_cnt;  /* LGM only */
} __attribute__((packed));

/**************************************************************************
 *! \struct system_stats_s
 **************************************************************************
 *
 * \brief system stats
 *
 **************************************************************************/
struct used_nodes_s {
	u32 tscd_num_of_used_nodes;
	u32 nodes_bmap[64];
} __attribute__((packed));

/**************************************************************************
 *! \struct	wsp_queue_t
 **************************************************************************
 *
 * \brief wsp queue helper in db
 * 	  for PRX only
 *
 **************************************************************************/
struct wsp_queue_t {
#if defined(__BIG_ENDIAN_BITFIELD)
	u32 reserved : 11,
	    suspended : 1,
	    rlm : 9,
	    node_id : 11;
#else /* Consider LE as default */
	u32 node_id:11,
	    rlm:9,
	    suspended:1,
	    reserved :11;
#endif
} __attribute__((packed));

/**************************************************************************
 *! \struct	wsp_stats_t
 **************************************************************************
 *
 * \brief wsp helper stats
 * 	  for PRX only
 *
 **************************************************************************/
struct wsp_stats_t {
	u32 num_timeouts;
	u32 num_toggles;
	u32 num_toggles_wm;
	u32 num_iterations;
	u32 num_iterations_wm;
} __attribute__((packed));

/**************************************************************************
 *! \union	wsp_queues_t
 **************************************************************************
 *
 * \brief wsp helper queues structure in host DB
 *
 **************************************************************************/
union wsp_queues_t {
	u32 queues_bitmap_lgm[QOS_MAX_NUM_QUEUES / 32];
	struct prx {
		u32 num_queues;
		struct wsp_queue_t queues[QOS_ENHANCED_WSP_MAX_QUEUES_PRX];
	} prx;
} __attribute__((packed));

/**************************************************************************
 *! \struct	bwl_ddr_info_t
 **************************************************************************
 *
 * \brief BW limit information
 *
 **************************************************************************/
struct bwl_ddr_info_t {
	u32    bw_limit;
	u32    max_burst;
} __attribute__((packed));

/**************************************************************************
 *! @enum UC_QOS_CMD
 **************************************************************************
 *
 * @brief UC QOS command enum. Must be synced with the Host definition
 *
 **************************************************************************/
enum uc_qos_command {
	UC_QOS_CMD_GET_FW_VERSION,
	UC_QOS_CMD_MULTIPLE_COMMANDS,
	UC_QOS_CMD_INIT_UC_LOGGER,
	UC_QOS_CMD_SET_UC_LOGGER_LEVEL,
	UC_QOS_CMD_INIT_QOS,
	UC_QOS_CMD_ADD_PORT,
	UC_QOS_CMD_REM_PORT,
	UC_QOS_CMD_ADD_SCHEDULER,
	UC_QOS_CMD_REM_SCHEDULER,
	UC_QOS_CMD_ADD_QUEUE,
	UC_QOS_CMD_REM_QUEUE,
	UC_QOS_CMD_FLUSH_QUEUE,
	UC_QOS_CMD_SET_PORT,
	UC_QOS_CMD_SET_SCHEDULER,
	UC_QOS_CMD_SET_QUEUE,
	UC_QOS_CMD_MOVE_SCHEDULER,
	UC_QOS_CMD_MOVE_QUEUE,
	UC_QOS_CMD_GET_PORT_STATS,
	UC_QOS_CMD_GET_QUEUE_STATS,
	UC_QOS_CMD_GET_SYSTEM_STATS,
	UC_QOS_CMD_ADD_SHARED_BW_LIMIT_GROUP,
	UC_QOS_CMD_REM_SHARED_BW_LIMIT_GROUP,
	UC_QOS_CMD_SET_SHARED_BW_LIMIT_GROUP,
	UC_QOS_CMD_GET_NODE_INFO,
	UC_QOS_CMD_DEBUG_READ_NODE,
	UC_QOS_CMD_DEBUG_PUSH_DESC,
	UC_QOS_CMD_DEBUG_POP_DESC,
	UC_QOS_CMD_DEBUG_MCDMA_COPY,
	UC_QOS_CMD_DEBUG_ADD_CREDIT_TO_PORT,
	UC_QOS_CMD_GET_ACTIVE_QUEUES_STATS,
	UC_QOS_CMD_UPDATE_PORT_TREE,
	UC_QOS_CMD_SET_AQM_SF,
	UC_QOS_CMD_SET_CODEL_CFG,
	UC_QOS_CMD_GET_CODEL_QUEUE_STATS,
	UC_QOS_CMD_GET_USED_NODES,
	UC_QOS_CMD_SUSPEND_PORT_TREE,
	UC_QOS_CMD_GET_QUANTA,
	UC_QOS_CMD_WSP_HELPER_SET,
	UC_QOS_CMD_WSP_HELPER_STATS_GET,
	UC_QOS_CMD_MOD_REG_BMAP_SET,
	UC_QOS_CMD_MOD_REG_BMAP_GET,
	UC_QOS_CMD_GET_HIST_STATS,
};

/**************************************************************************
 *! \enum	WRED_COMMAND_OPERATION
 **************************************************************************
 *
 * \brief WRED commands
 *
 **************************************************************************/
typedef enum {
	WRED_CMD_OP_READ_Q_COUNTERS, /*! Read Q counters */
	WRED_CMD_OP_READ_CLR_Q_COUNTERS, /*! Read-Clear Q counters */
	WRED_CMD_OP_REMOVE_Q, /*! Remove Q */
	WRED_CMD_OP_ADD_NEW_Q, /*! Add new Q */
	WRED_CMD_OP_UPDATE_Q, /*! Update Q */
} WRED_COMMAND_OPERATION;

/**************************************************************************
 *
 * @brief UC commands.
 * This structure defines the Host <-->UC interface
 *
 **************************************************************************/
struct uc_qos_cmd_base {
	//!< Type of command (UC_QOS_CMD)
	u32 type;

	//!< Commands flags
	u32 flags;
#define UC_CMD_FLAG_MULTIPLE_COMMAND_LAST BIT_ULL(0)
#define UC_CMD_FLAG_UC_FATAL              BIT_ULL(29)
#define UC_CMD_FLAG_UC_DONE               BIT_ULL(30)
#define UC_CMD_FLAG_UC_ERROR              BIT_ULL(31)

	//!< Number of 32bit parameters available for this command.
	/* must be synced between the host and uc! */
	u32 num_params;
} __attribute__((packed));

struct fw_cmd_multiple_cmds {
	struct uc_qos_cmd_base base;
	u32 next_cmd_addr;
} __attribute__((packed));

struct fw_cmd_get_version {
	struct uc_qos_cmd_base base;
	u32 major;
	u32 minor;
	u32 build;
} __attribute__((packed));

struct fw_cmd_init_logger {
	struct uc_qos_cmd_base base;
	u32 write_idx_addr; /*! Logger write index address */
	u32 read_idx_addr;  /*! Logger read index address */
	u32 ring_addr;      /*! Logger address */
	u32 mode;           /*! Logger mode */
	u32 level;          /*! Logger log level */
	u32 num_of_msgs;    /*! Total number of messages */
} __attribute__((packed));

struct fw_cmd_set_log_level {
	struct uc_qos_cmd_base base;
	u32 level;       /*! Logger log level */
} __attribute__((packed));

struct fw_cmd_init_qos {
	struct uc_qos_cmd_base base;
	u32 qm_ddr_start; /*! QM base address (in lgm, shifted by 4 bits) */
	u32 qm_num_pages; /*! QM number of DDR pages */
	u32 wred_total_avail_resources; /*! total available resources */
	u32 wred_prioritize_pop; /*! prioritize pop indication */
	u32 wred_avg_q_size_p; /*! average q size weight parameter */
	u32 wred_max_q_size; /*! max_queue_size_bytes (up to 2^25 bytes) */
	u32 num_of_ports; /*! Number of ports */
	u32 hw_clk; /*! QoS clock (In MHZ.) */
	u32 bm_base; /*! BM push address for CoDel */
	u32 soc_rev; /*! SOC revision (0 - A Step, 1 - B Step) */
	u32 bwl_ddr_base; /*! BWL temporary DDR base address */
	u32 sbwl_ddr_base; /*! SBWL temporary DDR base address */
	u32 wsp_queues_ddr_base; /*! WSP queues list */
	u32 mod_reg_log_en; /*! modules registers logs enable bitmap */
	u32 tbm_prescale_addr; /*! TBM prescale register address */
	u32 chk_crawler_prescale_addr; /*! checker crawler prescale register address */
	u32 egress_uc_prescale_addr; /*! egress uc prescale register address */
	u32 codel_interval; /*! codel interval time [mSec] */
	u32 codel_target; /*! codel target delay [mSec] */
	u32 egress_uc_aqm_info_addr; /*! egress uc AQM info address */
} __attribute__((packed));

struct fw_cmd_port_params {
	u32 first_child; /*! first child */
	u32 last_child; /*! last child */
	u32 bw_limit; /*! bw_limit (0 - No BW limit) */
	u32 actual_bw_limit; /*! [OUT]: actual bw limit returned */
	u32 best_effort; /*! best_effort_enable */
	u32 first_wrr_node; /*! first WRR node (0 if no WRR node) */
	u32 shared_bwl_entry; /*! Shared BWL entry (0 if no group defined) */
	u32 max_burst; /*! Defines the max quantas that can be accumulated (num quantas = 1 << (max_burst)) */
} __attribute__((packed));

struct fw_cmd_port_ring_params {
	u32 txmgr_ring_sz; /*! TX manager ring size */
	u32 txmgr_ring_addr_h; /*! TX manager ring address high */
	u32 txmgr_ring_addr_l; /*! TX manager ring address low */
} __attribute__((packed));

struct fw_cmd_add_port {
	struct uc_qos_cmd_base base;
	u32 port_phy; /*! Port ID */
	u32 active; /*! Active port (0-Inactive/1-Active) */
	struct fw_cmd_port_params params;
	u32 disable_byte_credit; /*! Disable byte credit (0 Byte, 1 packet) */
	struct fw_cmd_port_ring_params ring_params;
	u32 txmgr_initial_port_credit; /*! TX manager initial port credit */
	u32 green_thr; /*! Green threshold per port */
	u32 yellow_thr; /*! Yellow threshold per port */
} __attribute__((packed));

struct fw_cmd_add_sched {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Sched node ID */
	u32 first_child; /*! first child */
	u32 last_child; /*! last child */
	u32 bw_limit; /*! bw_limit (0 - No BW limit) */
	u32 actual_bw_limit; /*! [OUT]: actual bw limit returned */
	u32 best_effort; /*! best_effort_enable */
	u32 first_wrr_node; /*! first WRR node (0 if no WRR node) */
	u32 shared_bwl_entry; /*! Shared BWL entry (0 if no group defined) */
	u32 max_burst; /*! Defines the max quantas that can be accumulated (num quantas = 1 << (max_burst)) */
	u32 preds[QOS_MAX_PREDECESSORS];
} __attribute__((packed));

#define WRED_FLAGS_DISABLE_WRED           BIT_ULL(0)
#define WRED_FLAGS_DISABLE_MIN_GUARANTEED BIT_ULL(1)
#define WRED_FLAGS_DISABLE_QM_FULL        BIT_ULL(2)
#define WRED_FLAGS_FIXED_PROBABILITY      BIT_ULL(3)

struct fw_cmd_queue_params {
	u32 active; /*! active_q */

	/*! bit 0 - disable wred
	 * bit 1 - disable min guaranteed max allowed
	 * bit 2 - disable qm_full
	 * bit 3 - use fixed probability instead of sloped area in wred graph
	 */
	u32 disable_flags;
	u32 fixed_green_drop_p; /*! fixed_green_drop_p */
	u32 fixed_yellow_drop_p; /*! fixed_yellow_drop_p */
	u32 min_avg_yellow; /*! min_avg_yellow */
	u32 max_avg_yellow; /*! max_avg_yellow */
	u32 slope_yellow; /*! slope_yellow (0-100 scale) */
	u32 min_avg_green; /*! min_avg_green */
	u32 max_avg_green; /*! max_avg_green */
	u32 slope_green; /*! slope_green (0-100 scale) */
	u32 min_guaranteed; /*! min_guaranteed */
	u32 max_allowed; /*! max_allowed */
	u32 is_alias; /*! is_alias */
	u32 codel_en; /*! TX manager CoDel enable */
	u32 is_fast_q; /*! Enable double pages to cache for this q */
} __attribute__((packed));

struct fw_cmd_add_queue {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Queue ID */
	u32 port; /*! Port ID */
	u32 rlm; /*! Physical Queue ID */
	u32 bw_limit; /*! bw_limit (0 - No BW limit) */
	u32 actual_bw_limit; /*! [OUT]: actual bw limit returned */
	u32 shared_bwl_entry; /*! Shared BWL entry (0 - no Shared BW group) */
	u32 max_burst; /*! Defines the max quantas that can be accumulated (num quantas = 1 << (max_burst)) */
	u32 preds[QOS_MAX_PREDECESSORS]; /*! Predecessors */
	struct fw_cmd_queue_params params;
} __attribute__((packed));

struct fw_cmd_set_sched {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Sched node ID */
	u32 valid; /*! Valid fields */
	u32 suspend; /*! Sched suspended */
	u32 first_child; /*! first child */
	u32 last_child; /*! last child */
	u32 bw_limit; /*! bw_limit (0 - No BW limit) */
	u32 actual_bw_limit; /*! [OUT]: actual bw limit returned */
	u32 best_effort; /*! best_effort_enable */
	u32 first_wrr_node; /*! first WRR node (0 if no WRR node) */
	u32 node_weight; /*! Set 2-1-1 if first node is double weighted */
	u32 shared_bwl_entry; /*! Shared BWL entry (0 if no group defined) */
	u32 max_burst; /*! Defines the max quantas that can be accumulated (num quantas = 1 << (max_burst)) */
	u32 preds[QOS_MAX_PREDECESSORS];
} __attribute__((packed));

struct fw_cmd_set_port {
	struct uc_qos_cmd_base base;
	u32 port_phy; /*! Port ID */
	u32 valid; /*! Valid fields */
	u32 suspend; /*! Port suspended */
	struct fw_cmd_port_params params;
	u32 port_valid; /*! Port valid fields */
	struct fw_cmd_port_ring_params ring_params;
	u32 active; /*! Active port (0-Inactive/1-Active) */
	u32 green_thr; /*! Green threshold per port */
	u32 yellow_thr; /*! Yellow threshold per port */
} __attribute__((packed));

struct fw_cmd_set_queue {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Queue ID */
	u32 rlm; /*! Physical Queue ID */
	u32 valid; /*! Node Valid fields bitmap */
	u32 suspend; /*! Suspend node (0-resume/1-suspend) */
	u32 bw_limit; /*! bw_limit (0 - No BW limit) */
	u32 actual_bw_limit; /*! [OUT]: actual bw limit returned */
	u32 node_weight; /*! Set 2-1-1 if first node is double weighted */
	u32 shared_bwl_entry; /*! Shared BWL entry (0 - no Shared BW group) */
	u32 max_burst; /*! Defines the max quantas that can be accumulated (num quantas = 1 << (max_burst)) */
	u32 preds[QOS_MAX_PREDECESSORS]; /*! Predecessors */
	u32 port; /*! Port ID */
	u32 queue_valid; /*! Queue valid bitmap defined in enum queue_conf */
	struct fw_cmd_queue_params params;
} __attribute__((packed));

struct fw_cmd_move_sched {
	struct uc_qos_cmd_base base;
	u32 src; /*! Src Node */
	u32 dst; /*! Dst Node */
	u32 preds[QOS_MAX_PREDECESSORS]; /*! Dst Node Predecessors */
} __attribute__((packed));

struct fw_cmd_move_queue {
	struct uc_qos_cmd_base base;
	u32 src; /*! Src Node */
	u32 dst; /*! Dst Node */
	u32 dst_port; /*! Dst Port */
	u32 rlm; /*! Physical queue */
	u32 preds[QOS_MAX_PREDECESSORS]; /*! Dst Node Predecessors */
	u32 is_alias; /*! is_alias */
} __attribute__((packed));

struct fw_cmd_remove_queue {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Queue ID */
	u32 rlm; /*! Physical Queue ID */
	u32 is_alias; /*! Queue is aliased */
	u32 is_fast_q; /*! Is fast Queue */
} __attribute__((packed));

struct fw_cmd_remove_port {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Port ID */
} __attribute__((packed));

struct fw_cmd_remove_sched {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Sched ID */
} __attribute__((packed));

struct fw_cmd_set_aqm_sf {
	struct uc_qos_cmd_base base;
	u32 aqm_context; /*! Aqm Context (0 - 15) */
	u32 enable; /*! Enable */
	u32 latency_target_ms; /*! Latency Target (milliseconds) */
	u32 peak_rate; /*! Peak Rate (Bytes/seconds) */
	u32 msr; /*! MSR (Bytes/seconds) */
	u32 buffer_size; /*! Buffer Size */
	u32 msrtokens_addr_offset; /*! msrtokens dccm reg addr */
	u32 num_rlms; /*! Number of queues */
	u32 rlms[QOS_AQM_CONTEXT_MAX_QUEUES]; /*! queue_id's */
	u32 llsf; /*! [LLD] is LL SF */
	u32 coupled_sf; /*! [LLD] this is the coupled SF (WRED_AQM_NUM_CONTEXTS otherwise) */
	u32 coupling_factor; /*! [LLD] coupling factor */
	u32 amsr; /*! [LLD] aggregated SF MSR (Bits/seconds) */
	u32 msr_l; /*! [LLD] MSR of the LL SF (Bits/seconds) */
	u32 weight; /*! [LLD] Scheduling Weight / 256 */
	u32 num_hist_bins; /*! Histogram - Num bins. Set 0 to disable Histogram */
	u32 bin_edges[QOS_AQM_MAX_BINS - 1]; /*! Histogram - Bin edges */
} __attribute__((packed));

struct fw_cmd_update_port_tree {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Port ID */
	u32 max_allowed_addr; /*! Max allowed buffer ddr address */
	u32 bitmap[QOS_MAX_NUM_QUEUES / 32]; /*! Port Queue's bitmap words */
} __attribute__((packed));

struct fw_cmd_add_bwl_group {
	struct uc_qos_cmd_base base;
	u32 id; /*! Shared BW limit group ID (supported values: 1-511) */
	u32 max_burst; /*! burst size (number of quantas = 1 << max_burst) */
} __attribute__((packed));

struct fw_cmd_set_bwl_group {
	struct uc_qos_cmd_base base;
	u32 id; /*! Shared BW limit group ID (supported values: 1-511) */
	u32 max_burst; /*! burst size (number of quantas = 1 << max_burst) */
} __attribute__((packed));

struct fw_cmd_rem_bwl_group {
	struct uc_qos_cmd_base base;
	u32 id; /*! Shared BW limit group ID (supported values: 1-511) */
} __attribute__((packed));

struct fw_cmd_set_codel {
	struct uc_qos_cmd_base base;
	u32 target_delay_msec; /*! Target delay [mSec] */
	u32 interval_time_msec; /*! Interval time [mSec] */
} __attribute__((packed));

struct fw_cmd_push_desc {
	struct uc_qos_cmd_base base;
	u32 rlm; /*! Queue ID */
	u32 size; /*! packet size */
	u32 color; /*! Packet color (#WRED_PACKET_COLOR) */
	u32 addr; /*! Packet address */
	u32 policy; /*! Policy */
	u32 pool; /*! Pool */
	u32 gpid; /*! Tx port ID */
	u32 data_offset; /*! Data offset */
} __attribute__((packed));

struct fw_cmd_pop_desc {
	struct uc_qos_cmd_base base;
	u32 rlm; /*! Queue ID */
	u32 addr; /*! Descriptor address */
} __attribute__((packed));

struct fw_cmd_read_table_entry {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Node ID */
	u32 table_type; /*! table type (#TSCD_TABLE_TYPE) */
	u32 addr; /*! Output address to write the entry */
} __attribute__((packed));

struct fw_cmd_flush_queue {
	struct uc_qos_cmd_base base;
	u32 rlm; /*! Physical Queue ID */
} __attribute__((packed));

struct fw_cmd_get_queue_stats {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Node ID */
	u32 rlm; /*! Physical Queue ID */
	u32 addr; /*! Address to write the statistics to (#queue_stats_t) */
	u32 reset; /*! Clear WRED stats after read */
} __attribute__((packed));

struct fw_cmd_get_codel_stats {
	struct uc_qos_cmd_base base;
	u32 rlm; /*! Physical Queue ID */
	u32 addr; /*! Address to write the statistics to (#queue_stats_t) */
	u32 reset; /*! Clear CoDel stats after read */
} __attribute__((packed));

struct fw_cmd_get_port_stats {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Port ID */
	u32 addr; /*! Address to write the statistics to (#queue_stats_t) */
	u32 reset; /*! Clear port stats after read */
} __attribute__((packed));

struct fw_cmd_get_hist_stats {
	struct uc_qos_cmd_base base;
	u32 sf; /*! Service flow */
	u32 addr; /*! Address to write the statistics to (#hist_stats_t) */
	u32 reset; /*! Clear stats after read */
} __attribute__((packed));

struct fw_cmd_get_system_info {
	struct uc_qos_cmd_base base;
	u32 addr; /*! Address to write the statistics to (#system_stats_t) */
	u32 dump; /*! Dump used entries */
} __attribute__((packed));

struct fw_cmd_get_used_nodes {
	struct uc_qos_cmd_base base;
	u32 addr; /*! Address to write the statistics to (#system_stats_t) */
	u32 dump; /*! Dump used entries */
} __attribute__((packed));

struct fw_cmd_get_node_info {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Node ID */
	u32 addr; /*! Address to write the statistics to (#hw_node_info_t) */
} __attribute__((packed));

struct fw_cmd_get_qm_stats {
	struct uc_qos_cmd_base base;
	u32 first_rlm; /*! Physical ID of the first queue */
	u32 last_rlm; /*! Physical ID of the last queue */
	/*! Address to write the statistics to. Written in this format:
	 * #num_queues (4 bytes) and then for each queue:
	 * (4 bytes queue id)(stats as in #queue_stats_t)
	 */
	u32 addr;
} __attribute__((packed));

struct fw_cmd_suspend_port_tree {
	struct uc_qos_cmd_base base;
	u32 phy; /*! Port ID */
} __attribute__((packed));

struct fw_cmd_add_credit_to_port {
	struct uc_qos_cmd_base base;
	u32 port_phy; /* Port ID */
	u32 disable_byte_credit; /* 0-Byte credit, 1-Packet credit */
	u32 credits; /* Credit to add */
} __attribute__((packed));

struct fw_cmd_get_quanta {
	struct uc_qos_cmd_base base;
	u32 quanta;   /*! [OUT]: the quanta in KB */
} __attribute__((packed));

struct fw_cmd_mod_log_bmap {
	struct uc_qos_cmd_base base;
	u32 bmap;
} __attribute__((packed));

struct fw_cmd_mcdma_copy {
	struct uc_qos_cmd_base base;
	u32 src_addr; /*! Source address */
	u32 dst_addr; /*! Destination address */
	u32 size; /*! Size in bytes */
} __attribute__((packed));

struct wsp_helper_cfg {
	/* ! for LGM delay between 2 enabled cycles,
	for prx timeout_microseconds */
	u32  timeout;
	u32  byte_threshold_prx;
	u32  enable_prx;
} __attribute__((packed));

struct fw_cmd_wsp_helper {
	struct uc_qos_cmd_base base;
	struct wsp_helper_cfg cfg;
} __attribute__((packed));

struct fw_cmd_wsp_helper_stats {
	struct uc_qos_cmd_base base;
	u32 reset; /* Reset statistics */
} __attribute__((packed));

union uc_qos_cmd_s {
	struct uc_qos_cmd_base           base;
	struct fw_cmd_multiple_cmds      multiple_cmds;
	struct fw_cmd_get_version        get_version;
	struct fw_cmd_init_logger        init_logger;
	struct fw_cmd_set_log_level      set_log_level;
	struct fw_cmd_init_qos           init_qos;
	struct fw_cmd_add_port           add_port;
	struct fw_cmd_add_sched          add_sched;
	struct fw_cmd_add_queue          add_queue;
	struct fw_cmd_set_sched          set_sched;
	struct fw_cmd_set_port           set_port;
	struct fw_cmd_set_queue          set_queue;
	struct fw_cmd_move_sched         move_sched;
	struct fw_cmd_move_queue         move_queue;
	struct fw_cmd_remove_queue       remove_queue;
	struct fw_cmd_remove_port        remove_port;
	struct fw_cmd_remove_sched       remove_sched;
	struct fw_cmd_set_aqm_sf         set_aqm_sf;
	struct fw_cmd_update_port_tree   update_port_tree;
	struct fw_cmd_add_bwl_group      add_bwl_group;
	struct fw_cmd_set_bwl_group      set_bwl_group;
	struct fw_cmd_rem_bwl_group      rem_bwl_group;
	struct fw_cmd_set_codel          set_codel;
	struct fw_cmd_push_desc          push_desc;
	struct fw_cmd_pop_desc           pop_desc;
	struct fw_cmd_mcdma_copy         mcdma_copy;
	struct fw_cmd_read_table_entry   read_tbl_entry;
	struct fw_cmd_flush_queue        flush_queue;
	struct fw_cmd_get_queue_stats    get_queue_stats;
	struct fw_cmd_get_codel_stats    get_codel_stats;
	struct fw_cmd_get_port_stats     get_port_stats;
	struct fw_cmd_get_system_info    get_sys_info;
	struct fw_cmd_get_node_info      get_node_info;
	struct fw_cmd_get_qm_stats       get_qm_stats;
	struct fw_cmd_suspend_port_tree  suspend_port_tree;
	struct fw_cmd_add_credit_to_port add_credit_to_port;
	struct fw_cmd_get_used_nodes     get_used_nodes;
	struct fw_cmd_wsp_helper         set_wsp_helper;
	struct fw_cmd_wsp_helper_stats   get_wsp_helper_stats;
	struct fw_cmd_mod_log_bmap       mod_log_bmap;
	struct fw_cmd_get_hist_stats     get_hist_stats;
} __attribute__((packed));

typedef struct {
	u32 errors;
	u32 fatal;
	u32 warnings;
	u32 debug[64];
} __attribute__((packed)) debug_counters_t;

typedef struct {
	u32 wakeup;
	u32 version;
	u32 mod_reg_log_en;
	u32 uc_log_mode;
	u32 gaurd_offset;
	u32 cmd_time;
	u32 cmds_in_burst;
	u32 cmd_count;
	u32 cmd_buf_offset;
	u32 cmd_rsp_offset;
	u32 stack_ptr;
	u32 cmd_rsp_sz;
	u32 cmd_buf_size;
	u32 stack_size;
	u32 num_updates_per_sec;
	u32 quanta;
	u32 cnt_offset;
	u32 aqm_offset;
	u32 wsp_updated;
} __attribute__((packed)) shared_db_t;


typedef struct tscd_branch_table_entry_s {
	u32	predecessor0;
	u32	predecessor1;
	u32	predecessor2;
	u32	predecessor3;
	u32	predecessor4;
	u32	predecessor5;
} __attribute__((aligned(4))) tscd_branch_table_entry_t;

typedef struct host_tscd_rl_map_table_entry_s {
	u32	credit_remainder;
	u32	leaf_node;
	u32	vsi;
	u32	pf_num;
	u32	vm_vf_num;
	u32	vm_vf_type;
	u32	port;
	u32	tc_num;
	u32	suspend;
	u32	res;
	u32	lowlatency;
	u32	wa_select;
	u32	pe_wa;
	u32	lan_wa;
	u32	push_credit_req;
	u32	ll_credit_req;
} __attribute__((aligned(4))) host_tscd_rl_map_table_entry_t;

typedef struct tscd_bwl_table_entry_s {
	u32	bwl_credit_avail;
	u32	bwl_credit_inc;
	u32	bwl_credit_max;
	u32	node_update_pending;
	u32	sbwl_entry_idx;
} __attribute__((aligned(4))) tscd_bwl_table_entry_t;

typedef struct tscd_shared_bwl_table_entry_s {
	u32	shared_bwl_credit_avail;
	u8	shared_bwl_credit_max;
} __attribute__((aligned(4))) tscd_shared_bwl_table_entry_t;

typedef struct host_tscd_node_table_entry_s {
	u32	last_child;
	u32	first_child;
	u32	branch_table_idx;
	u32	next_sch_wrr;
	u32	wsp_wrr_switch;
	u32	tc_wa;
	u32	bw_limit;
	u32	suspend;
	u32	credit_toggle;
	u32	bw_limit_en;
	u32	best_effort_en;
	u32	child_round_toggle;
	u32	credit_avail;
	u32	credit_inc;
	u32	small_child_round_toggle;
	u32	small_credit_toggle;
	u32	small_credit_avail;
	u32	small_credit_inc;
} __attribute__((aligned(4))) host_tscd_node_table_entry_t;

union host_read_entry_t{
	host_tscd_node_table_entry_t node_entry;
	tscd_bwl_table_entry_t bwl_entry;
	host_tscd_rl_map_table_entry_t rlm_entry;
	tscd_branch_table_entry_t branch_entry;
	tscd_shared_bwl_table_entry_t shared_bwl_entry;
};

#endif /* SRC_UC_HOST_DEFS_H_ */
