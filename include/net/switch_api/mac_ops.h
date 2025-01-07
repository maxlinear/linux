/******************************************************************************
 *                Copyright (C) 2020-2024 MaxLinear, Inc.
 *                Copyright (c) 2016-2020 Intel Corporation
 *
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.
 *
 ******************************************************************************/

#ifndef _MAC_OPS_H_
#define _MAC_OPS_H_

#include "gsw_irq.h"
#include "gsw_types.h"
#include "lantiq_gsw.h"
#ifdef __KERNEL__
#include <linux/netdevice.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#else
#ifndef phy_interface_t
typedef int phy_interface_t;
#endif
#endif

enum {
	MODE0 = 0,
	MODE1,
	MODE2,
	MODE3
};

struct lmac_rmon_cnt {
	u32 sing_coln_cnt;
	u32 mple_coln_cnt;
	u32 late_coln_cnt;
	u32 excs_coln_cnt;
	u32 rx_pause_cnt;
	u32 tx_pause_cnt;
};

struct xgmac_mmc_stats {
	/* Tx Stats */
	u64 txoctetcount_gb;
	u64 txframecount_gb;
	u64 txbroadcastframes_g;
	u64 txmulticastframes_g;
	u64 txunicastframes_gb;
	u64 txmulticastframes_gb;
	u64 txbroadcastframes_gb;
	u64 txunderflowerror;
	u64 txoctetcount_g;
	u64 txframecount_g;
	u64 txpauseframes;
	u64 txvlanframes_g;

	/* Rx Stats */
	u64 rxframecount_gb;
	u64 rxoctetcount_gb;
	u64 rxoctetcount_g;
	u64 rxbroadcastframes_g;
	u64 rxmulticastframes_g;
	u64 rxcrcerror;
	u64 rxundersize_g;
	u64 rxoversize_g;
	u64 rxunicastframes_g;
	u64 rxlengtherror;
	u64 rxoutofrangetype;
	u64 rxpauseframes;
	u64 rxfifooverflow;
};

enum {
	RMON_TX_PKTS = 0,
	RMON_TX_BYTES,
	RMON_TX_GOOD_PKTS,
	RMON_TX_GOOD_BYTES,
	RMON_TX_PAUSE,
	RMON_TX_UNDERFLOW,
	RMON_TX_SINGLE_COL,
	RMON_TX_MULTI_COL,
	RMON_TX_LATE_COL,
	RMON_TX_EXCS_COL,

	RMON_RX_PKTS,
	RMON_RX_BYTES,
	RMON_RX_GOOD_BYTES,
	RMON_RX_PAUSE,
	RMON_RX_CRC_ERR,
	RMON_RX_OVERFLOW,

	MAC_RMON_MAX
};

struct mac_rmon {
	u64 cnt[MAC_RMON_MAX];
};

/* This structure contains flags that indicate what hardware features
 * or configurations are present in the device.
 */
struct xgmac_hw_features {
	/* HW Version */
	u32 version;

	/* HW Feature Register0 */
	u32 gmii;              /* 1000 Mbps support */
	u32 vlhash;            /* VLAN Hash Filter */
	u32 sma;               /* SMA(MDIO) Interface */
	u32 rwk;               /* PMT remote wake-up packet */
	u32 mgk;               /* PMT magic packet */
	u32 mmc;               /* RMON module */
	u32 aoe;               /* ARP Offload */
	u32 ts;                /* IEEE 1588-2008 Advanced Timestamp */
	u32 eee;               /* Energy Efficient Ethernet */
	u32 tx_coe;            /* Tx Checksum Offload */
	u32 rx_coe;            /* Rx Checksum Offload */
	u32 addn_mac;          /* Additional MAC Addresses */
	u32 ts_src;            /* Timestamp Source */
	u32 sa_vlan_ins;       /* Source Address or VLAN Insertion */
	u32 vxn;				/* VxLAN/NVGRE Support */
	u32 ediffc;			/* Different Descriptor Cache */
	u32 edma;				/* Enhanced DMA */

	/* HW Feature Register1 */
	u32 rx_fifo_size;       /* MTL Receive FIFO Size */
	u32 tx_fifo_size;       /* MTL Transmit FIFO Size */
	u32 osten;              /* One-Step Timestamping Enable */
	u32 ptoen;              /* PTP Offload Enable */
	u32 adv_ts_hi;          /* Advance Timestamping High Word */
	u32 dma_width;          /* DMA width */
	u32 dcb;                /* DCB Feature */
	u32 sph;                /* Split Header Feature */
	u32 tso;                /* TCP Segmentation Offload */
	u32 dma_debug;          /* DMA Debug Registers */
	u32 rss;                /* Receive Side Scaling */
	u32 tc_cnt;             /* Number of Traffic Classes */
	u32 hash_table_size;    /* Hash Table Size */
	u32 l3l4_filter_num;    /* Number of L3-L4 Filters */

	/* HW Feature Register2 */
	u32 rx_q_cnt;           /* Number of MTL Receive Queues */
	u32 tx_q_cnt;           /* Number of MTL Transmit Queues */
	u32 rx_ch_cnt;          /* Number of DMA Receive Channels */
	u32 tx_ch_cnt;          /* Number of DMA Transmit Channels */
	u32 pps_out_num;        /* Number of PPS outputs */
	u32 aux_snap_num;       /* Number of Aux snapshot inputs */
};

/* GSWIP Tx Fifo to send signal from GSWIP to Xgmac
 * to do the action on the packet
 * Actions include
 * 1) Store Tx Timestamp
 * 2) One Step TImestamp update for Sync packets
 * 3) Packet ID update for the stored timestamp
 * 4) Timestamp correction needed
 * Entry will be cleared by the HW when packet is sent out
 */
struct mac_fifo_entry {
	u32 is_used;
	/* RecordID, denotes index to the 64 entry Fifo */
	u32 rec_id;
	/* Transmit Timestamp Store Enable */
	u8 ttse;
	/* One Step Timestamp Capture Enable */
	u8 ostc;
	/* One Step Timestamp Available/PacketID Available */
	u8 ostpa;
	/* Checksum Insertion or Update information */
	u8 cic;
	/* Lower 32 byte of time correction */
	u32 ttsl;
	/* Upper 32 byte of time correction */
	u32 ttsh;
	/* Timeout for this Fifo Entry */
	u32 timeout;
	u32 jiffies;
#ifdef __KERNEL__
	struct timer_list timer;
#endif
	struct sk_buff **ptp_tx_skb_loc;
};

struct mac_eip160_ops {
	struct platform_device *ig_pdev;
	struct platform_device *eg_pdev;

	int (*reg_rd)(struct platform_device *pdev, u32 off, u32 *pdata);
	int (*reg_wr)(struct platform_device *pdev, u32 off, u32 data);
};

/** \brief MAC Cli struct.
     MAC Cli struct for passing args and argument values. */
typedef struct {
	/** Command Type */
	u32 cmdType;
	/** Number of args */
	u32 argc;
	/** Set/Get Api */
	u32 get;
	/** Set All using "*" */
	u32 set_all;
	/** Mac Index going to set/get */
	u32 mac_idx;
	/** Mac Adddress string */
	u8 mac_addr[6];
	/** Argument values */
	u32 val[MAX_MAC_CLI_ARG];
	/** Reading Xgmac RMON Counters */
	struct xgmac_mmc_stats pstats;
	/** Reading Xgmac Capability */
	struct xgmac_hw_features hw_feat;
	/** 64 bit system time */
	u64 time;

	struct mac_fifo_entry f_entry;

	struct lmac_rmon_cnt lpstats;
} GSW_MAC_Cli_t;

/* RX THRESHOLD operations */
#define MTL_RX_THRESHOLD_32		0x01
#define MTL_RX_THRESHOLD_64		0x00
#define MTL_RX_THRESHOLD_96		0x02
#define MTL_RX_THRESHOLD_128	0x03
#define MTL_TX_THRESHOLD_32		0x01
#define MTL_TX_THRESHOLD_64		0x00
#define MTL_TX_THRESHOLD_96		0x02
#define MTL_TX_THRESHOLD_128	0x03
#define MTL_TX_THRESHOLD_192	0x04
#define MTL_TX_THRESHOLD_256	0x05
#define MTL_TX_THRESHOLD_384	0x06
#define MTL_TX_THRESHOLD_512	0x07

#define MTL_ETSALG_WRR			0x00
#define MTL_ETSALG_WFQ			0x01
#define MTL_ETSALG_DWRR			0x02
#define MTL_RAA_SP				0x00
#define MTL_RAA_WSP				0x01

typedef enum {
	MAC_CLI_INIT = 0,
	MAC_CLI_RESET,
	MAC_CLI_MTL_TX,
	MAC_CLI_MTL_RX,
	MAC_CLI_FCTHR,
	MAC_CLI_SET_TS_ADDEND,
	MAC_CLI_GET_SYSTEM_TIME,
	MAC_CLI_GET_TX_TSTAMP,
	MAC_CLI_SET_TSTAMP_EN,
	MAC_CLI_SET_TSTAMP_DIS,
	MAC_CLI_TXTSTAMP_MODE,
	MAC_CLI_HW_TSTAMP, /* 10 */
	MAC_CLI_FLUSH_TX_Q,
	MAC_CLI_ERR_PKT_FWD,
	MAC_CLI_SET_INT,
	MAC_CLI_EN,
	MAC_CLI_SET_MAC_ADDR,
	MAC_CLI_SET_RX_MODE,
	MAC_CLI_SET_MTU,
	MAC_CLI_PFCTRL,
	MAC_CLI_SET_PF_FILTER,
	MAC_CLI_PAUSE_TX, /* 20 */
	MAC_CLI_SPEED,
	MAC_CLI_SET_DPX_MODE,
	MAC_CLI_CSUM_OFFLOAD,
	MAC_CLI_LB,
	MAC_CLI_EEE,
	MAC_CLI_CRC_STRIP,
	MAC_CLI_CRC_STRIP_ACS,
	MAC_CLI_IPG,
	MAC_CLI_MAGIC_PMT,
	MAC_CLI_MAGIC_RWK, /* 30 */
	MAC_CLI_PMT_GUCAST,
	MAC_CLI_EXTCFG,
	MAC_CLI_SET_RXTXCFG,
	MAC_CLI_GET_MISS_PKT_CNT,
	MAC_CLI_GET_UFLOW_PKT_CNT,
	MAC_CLI_GET_TSTAMP_STS,
	MAC_CLI_GET_TXTSTAMP_CNT,
	MAC_CLI_GET_TXTSTAMP_PKTID,
	MAC_CLI_SET_LINKSTS,
	MAC_CLI_SET_LPITX, /* 40 */
	MAC_CLI_SET_MDIO_CL,
	MAC_CLI_SET_MDIO_RD,
	MAC_CLI_SET_MDIO_RD_CNT,
	MAC_CLI_SET_MDIO_WR,
	MAC_CLI_SET_MDIO_INT,
	MAC_CLI_SET_FCSGEN,
	MAC_CLI_SET_PCH,
	MAC_CLI_SET_GINT,
	MAC_CLI_SET_PFC,
	MAC_CLI_SET_RXCRC, /* 50 */
	MAC_CLI_GET_FIFO,
	MAC_CLI_ADD_FIFO,
	MAC_CLI_DEL_FIFO,
	MAC_CLI_SET_EXT_TS_SRC,
	MAC_CLI_GET_RMON,
	MAC_CLI_GET_PCH_CRC_CNT,
	MAC_CLI_CLEAR_RMON,
	MAC_CLI_GET_CNTR_CFG,
	MAC_CLI_GET_PRIV_DATA,
	MAC_CLI_GET_HW_FEAT, /* 60 */
	MAC_CLI_GET_ALL,
	MAC_CLI_GET_MDIO_CL,
	MAC_CLI_REG_RD,
	MAC_CLI_REG_WR,
} MAC_CLI_CMDS;

typedef enum {
	LMAC_REG_WR,
	LMAC_REG_RD,
	LMAC_RMON,
	LMAC_CLR_RMON,
	LMAC_IF_MODE,
	LMAC_DPX,
	LMAC_XFCS,
	LMAC_FLOWCON,
	LMAC_IPG,
	LMAC_PREAMBLE,
	LMAC_DEFERMODE,
	LMAC_JPS,
	LMAC_LB,
	LMAC_TXER,
	LMAC_LPIMONIT,
	LMAC_PSTAT,
	LMAC_PISR,
	LMAC_PAUSE_SA_MODE,
} LMAC_CLI_CMDS;

typedef enum  {
	/* Adaption layer does not insert FCS */
	TX_FCS_NO_INSERT = 0,
	/* Adaption layer insert FCS */
	TX_FCS_INSERT,
	/* Reserved 1 */
	TX_FCS_RES1,
	/* Reserved 2 */
	TX_FCS_RES2,
	/* FDMA does not remove FCS */
	TX_FCS_NO_REMOVE,
	/* FDMA remove FCS */
	TX_FCS_REMOVE,
	/* Reserved 3 */
	TX_FCS_RES3,
	/* Reserved 4 */
	TX_FCS_RES4,
	/* Packet does not have special tag and special tag is not removed */
	TX_SPTAG_NOTAG,
	/* Packet has special tag and special tag is replaced */
	TX_SPTAG_REPLACE,
	/* Packet has special tag and special tag is not removed */
	TX_SPTAG_KEEP,
	/* Packet has special tag ans special tag is removed */
	TX_SPTAG_REMOVE,
	/* Packet does not have FCS and FCS is not removed */
	RX_FCS_NOFCS,
	/* Reserved */
	RX_FCS_RES,
	/* Packet has FCS and FCS is not removed */
	RX_FCS_NO_REMOVE,
	/* Packet has FCS and FCS is removed */
	RX_FCS_REMOVE,
	/* Packet does not have time stamp and time stamp is not inserted */
	RX_TIME_NOTS,
	/* Packet does not have time stamp and time stamp is inserted */
	RX_TIME_INSERT,
	/* Packet has time stamp and time stamp is not inserted */
	RX_TIME_NO_INSERT,
	/* Reserved */
	RX_TIME_RES,
	/* Packet does not have special tag and special tag is not inserted. */
	RX_SPTAG_NOTAG,
	/* Packet does not have special tag and special tag is inserted. */
	RX_SPTAG_INSERT,
	/* Packet has special tag and special tag is not inserted. */
	RX_SPTAG_NO_INSERT,
	/* Reserved */
	RX_SPTAG_RES,
} MAC_OPER_CFG;

/* MAC Index used by modules */
typedef enum {
	PMAC_0 = 0,
	PMAC_1,
	MAC_2,
	MAC_3,
	MAC_4,
	MAC_5,
	MAC_6,
	MAC_7,
	MAC_8,
	MAC_9,
	MAC_10,
	PMAC_2,
	MAC_LAST,
} MAC_IDX;

struct mac_ops {
	/* This function used to define Flow Control mode of XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	mode	0 - Auto Mode based on GPHY/XPCS link.
	 *				1 - Flow Ctrl enabled only in RX
	 *				2 - Flow Ctrl enabled only in TX
	 *				3 - Flow Ctrl enabled both in RX & TX
	 *				4 - Flow Ctrl disabled both in RX & TX
	 * return	OUT	0:Flow Ctrl operation Set Successfully
	 * return	OUT	!0:Flow Ctrl operation Set Error
	 */
	int (*set_flow_ctl)(void *ops, u32 mode);

	/* This function Sets the Mac Adress in xgmac.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	*mac_addr MAC source address to Set
	 * return	OUT	-1:	Source Address Set Error
	 */
	int (*set_macaddr)(void *ops, u8 *mac_addr);

	/* This function Enables/Disables Rx CRC check.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	disable	Disable=1, Enable=0
	 * return	OUT	-1:	Set Failed
	 */
	int (*set_rx_crccheck)(void *ops, u8 dis);

	/* This function configure treatment of special tag
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	mode	0 - packet does not have special tag
	 *				1 - packet has special tag and special tag is replaced
	 *				2 - packet has special tag and no modification
	 *				3 - packet has special tag and special tag is removed
	 * return	OUT	-1:	Set Failed
	 */
#define SPTAG_MODE_NOTAG	0
#define SPTAG_MODE_REPLACE	1
#define SPTAG_MODE_KEEP		2
#define SPTAG_MODE_REMOVE	3
	int (*set_sptag)(void *ops, u8 mode);

	/* This function used to Retrieve Flow Control mode of XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	mode	0 - Auto Mode based on GPHY/XPCS link.
	 *				1 - Flow Ctrl enabled only in RX
	 *				2 - Flow Ctrl enabled only in TX
	 *				3 - Flow Ctrl enabled both in RX & TX
	 *				4 - Flow Ctrl disabled both in RX & TX
	 * return	OUT  -1:	Flow Ctrl operation Get Error
	 */
	int (*get_flow_ctl)(void *ops);

	/* This function Retrieve the Flow Control status from XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	mode	0 - Auto Mode based on GPHY/XPCS link.
	 *				1 - Flow Ctrl enabled only in RX
	 *				2 - Flow Ctrl enabled only in TX
	 *				3 - Flow Ctrl enabled both in RX & TX
	 *				4 - Flow Ctrl disabled both in RX & TX
	 * return	OUT  -1:	Flow Ctrl operation Get Error
	 */
	int (*get_stat_flow_ctl)(void *ops);

	/* This function Resets the MAC module.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	reset	0 - reset deassert
	 *				1 - reset assert
	 *				2 - backup register, reset toggle, restore register
	 * return	OUT  0:	Reset of MAC module Done Successfully
	 * return	OUT  -1:	Reset of MAC module Error
	 */
	int (*mac_reset)(void *ops, u32 mode);

	/* This function Configures MAC Loopback.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	val	1 - Loopback Enable, 0 - Loopback Dis
	 * return	OUT  -1:	Loopback Set Error
	 */
	int (*mac_config_loopback)(void *ops, u32 en);

	/* This function Configures MAC IPG.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	IPG val	Value is from 0 - 7,
	 *			where 0 denotes the default 96 bits
	 * 			000:	96 bit
	 *			001:	128 bit
	 *			010:	160 bit
	 *			011:	192 bit
	 *			100:	224 bit
	 *			101 - 111: Reserved
	 * return	OUT  -1:	IPG Set Error
	 */
	int (*mac_config_ipg)(void *ops, u32 ipg);

	/* This function used to define Speed Control Mode of XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	speed(Mbps)
	 *			10		- 10Mbps
	 *			100		- 100Mbps
	 *			1000	- 1Gbps
	 *			25000	- 2.5Gbps
	 *			100000	- 10Gbps
	 * return	OUT  -1:	Speed Set Error
	 */
	int (*set_speed)(void *ops, u32 speed);

	/* This function Retrieve the Speed Control Mode from XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	speed(Mbps)
	 *			10		- 10Mbps
	 *			100		- 100Mbps
	 *			1000	- 1Gbps
	 *			25000	- 2.5Gbps
	 *			100000	- 10Gbps
	 * return	OUT  -1:	Speed Get Error
	 */
	int (*get_speed)(void *ops);

	/* This function Retrieve the Speed Control Status from XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	speed(Mbps)
	 *			10		- 10Mbps
	 *			100		- 100Mbps
	 *			1000	- 1Gbps
	 *			25000	- 2.5Gbps
	 *			100000	- 10Gbps
	 * return	OUT  -1:	Speed Get Error
	 */
	int (*get_stat_speed)(void *ops);

	/* This function used to define Duplex Mode of XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	duplex
	 *			0	- Full Duplex
	 *			1	- Half Duplex
	 *			2	- Auto
	 * return	OUT  -1:	Duplex Set Error
	 */
	int (*set_duplex)(void *ops, u32 duplex);

	/* This function Retrieve the Duplex Mode from XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	duplex
	 *			0	- Full Duplex
	 *			1	- Half Duplex
	 *			2	- Auto
	 * return	OUT  -1:	Duplex Get Error
	 */
	int (*get_duplex)(void *ops);

	/* This function Retrieve the Duplex Status from XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	duplex
	 *			0	- Full Duplex
	 *			1	- Half Duplex
	 */
	int (*get_stat_duplex)(void *ops);

	/* This function Configures the LPI
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	LPI EN
	 * 			0	-	Disable
	 *			1	-	Enable
	 * param[in/out]IN:	LPI Wait time for 100M in usec
	 * param[in/out]IN:	LPI Wait time for 1G in usec
	 * return	OUT	-1:	LPI Set Error
	 */
	int (*set_lpi)(void *ops, u32 en, u32 wait_100m, u32 wait_1g);

	/* This function Gets the LPI Enable/Disable
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	LPI EN
	 *			0	- Disable
	 *			1	- Enable
	 * return	OUT  -1:	LPI Get Error
	 */
	int (*get_lpi)(void *ops);

	/* This function Configures the MII Interface
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	MII Mode
	 *			0	- MII
	 *			2	- GMII
	 *			4	- XGMII
	 *			5	- LMAC GMII
	 * return	OUT  -1:	MII Interface Set Error
	 */
	int (*set_mii_if)(void *ops, u32 mii);

	/* This function Gets the MII Interface
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	MII Mode
	 *			0	- MII
	 *			2	- GMII
	 *			4	- XGMII
	 *			5	- LMAC GMII
	 * return	OUT  -1:	MII Interface Get Error
	 */
	int (*get_mii_if)(void *ops);

	/* This function used to define Link Status Mode of XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	Link
	 *	0	- LINK Force UP
	 *	1	- LINK Force DOWN
	 *	Any	- AUTO Mode
	 * return	OUT  -1:	Link Status Set Error
	 */
	int (*set_link_sts)(void *ops, u32 link);

	/* This function Retrieve Link Status Mode from XPCS/PHY.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	Link
	 *	0	- LINK Force UP
	 *	1	- LINK Force DOWN
	 *	Any	- AUTO Mode
	 * return	OUT  -1:	Link Status Get Error
	 */
	int (*get_link_sts)(void *ops);

	/* This function Retrieve the Link Status from XPCS/PHY
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	Link
	 *	0	- LINK Force UP
	 *	1	- LINK Force DOWN
	 */
	int (*get_stat_link_sts)(void *ops);

	/* This function Sets the MTU Configuration
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	MTU
	 *			Max MTU that can be set is 10000 for PRX300
	 * return	OUT	-1:	MTU set exceed the Max limit
	 */
	int (*set_mtu)(void *ops, u32 mtu);

	/* This function Gets the MTU Configuration
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	MTU
	 *			MTU configured
	 * return	OUT  -1:	MTU Get Error
	 */
	int (*get_mtu)(void *ops);

	/* This function Sets the Pause frame Source Address
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	*mac_addr MAC source address to Set
	 * param[in/out]IN:	mode
	 *			1 - PORT specific MAC source address
	 *			0 - COMMON MAC source address
	 * return	OUT	-1: Pause frame Source Address Set Error
	 */
	int (*set_pfsa)(void *ops, u8 *mac_addr, u32 mode);

	/* This function Gets the Pause frame Source Address
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	*mac_addr	MAC source address which is set
	 * return	OUT:	*mode		Mode configured
	 *			1 - PORT specific MAC source address
	 *			0 - COMMON MAC source address
	 * return	OUT	-1:	Pause frame Source Address Get Error
	 */
	int (*get_pfsa)(void *ops, u8 *mac_addr, u32 *mode);

	/* This function Sets the FCS generation Configuration
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	val	FCS generation Configuration
	 *			0 - CRC and PAD insertion are enabled.
	 *			1 - CRC insert enable and PAD insert disable
	 *			2 - CRC and PAD are not insert and not replaced.
	 * return	OUT	-1:	FCS generation Set Error
	 */
	int (*set_fcsgen)(void *ops, u32 fcsgen);

	/* This function Gets the FCS generation Configuration
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	val	FCS generation Configuration
	 *			0 - CRC and PAD insertion are enabled.
	 *			1 - CRC insert enable and PAD insert disable
	 *			2 - CRC and PAD are not insert and not replaced.
	 * return	OUT	-1:	FCS generation Get Error
	 */
	int (*get_fcsgen)(void *ops);

	/* This function Clears MAC Interrupt Status
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	event	Difffernt events to clear
	 * return	OUT	: Cleared return status
	 */
	int (*clr_int_sts)(void *ops, u32 event);

	/* This function Gets MAC Interrupt Status
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	:	Interrupts Pending
	 */
	int (*get_int_sts)(void *ops);

	/* This function Initializes System time Configuration
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	sec	Initial seconds value to be configured
	 *				in register
	 * param[in/out]IN:	nsec	Initial nano-seconds value to be
	 *				configured in register
	 * return	OUT	-1: System time Configuration Set Error
	 */
	int (*init_systime)(void *ops, u64 sec, u32 nsec);

	/* This function Configures addend value
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	addend
	 * The Timestamp Addend register is present only when the IEEE 1588
	 * Timestamp feature is selected without
	 * external timestamp input. This register value is used only when the
	 * system time is configured for Fine
	 * Update mode (TSCFUPDT bit in the MAC_Timestamp_Ctrl register).
	 * The content of this register is
	 * added to a 32-bit accumulator in every clock cycle and the
	 * system time is updated whenever the accumulator overflows.
	 * This field indicates the 32-bit time value to be added to the
	 * Accumulator register to achieve time synchronization.
	 * return	OUT	-1: Addend Configuration Set Error
	 */
	int (*config_addend)(void *ops, u32 addend);

	/* This function Adjust System Time for PTP
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	sec	New seconds value to be configured
	 * param[in/out]IN:	nsec	New nano seconds value to be configured
	 * param[in/out]IN:	addsub	Add or Subtract Time
	 * When this bit is set, the time value is subtracted with the
	 * contents of the update register. When this bit is reset,
	 * the time value is added with the contents of the update register.
	 * param[in/out]IN:	one_nsec_accuracy
	 *	If the new nsec value need to be subtracted with
	 *	the system time, then MAC_STNSUR.TSSS field should be
	 *	programmed with,
	 *	(10^9 - <new_nsec_value>) if MAC_TX_CFG.TSCTRLSSR is set or
	 *	(2^31 - <new_nsec_value> if MAC_TX_CFG.TSCTRLSSR is reset)
	 * return   OUT -1:     Adjust System Time for PTP Error
	 */
	int (*adjust_systime)(void *ops, u32 sec, u32 nsec, u32 addsub,
			      u32 adjust_systime);

	/* This sequence is used get 64-bit system time in nano sec
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	u64: 64-bit system time in nano sec
	 */
	u64 (*get_systime)(void *ops);

	/* This sequence is used get Transmitted 64-bit system time in nano sec
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	u64: Tx 64-bit system time in nano sec
	 */
	u64 (*get_tx_tstamp)(void *ops);

	/* This sequence is used get Tx Transmitted capture count
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	u32: Tx Timestamp capture count
	 */
	int (*get_txtstamp_cap_cnt)(void *ops);

	/* This sequence is used Configure HW TimeStamping TX/RX filter Cfg
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	tx_type: 1/0 - ON/OFF
	 * param[in/out]IN:	rx_filter:	This is based on Table 7-146,
	 *			Receive side timestamp Capture scenarios
	 * return	OUT	-1:	Configure HW TimeStamping Set Error
	 */
	int (*config_hw_time_stamping)(void *ops, u32 tx_type, u32 rx_filter);

	/* This sequence is used ConfigureSub Second Increment
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	ptp_clk: PTP Clock Value in Hz
	 *	The value programmed in this field is accumulated with the
	 *	contents of the sub-second register.
	 *	For example, when the PTP clock is 50 MHz (period is 20 ns),
	 *	you should program 20 (0x14)
	 *	when the System TimeNanoseconds register has an accuracy
	 *	of 1 ns [Bit 9 (TSCTRLSSR) is set in MAC_Timestamp_Ctrl].
	 *	When TSCTRLSSR is clear, the Nanoseconds register has a
	 *	resolution of ~0.465 ns. In this case, you
	 *	should program a value of 43 (0x2B)
	 *	which is derived by 20 ns/0.465.
	 * return	OUT	-1: Configure Sub Second Inccrement Error
	 */
	int (*config_subsec_inc)(void *ops, u32 ptp_clk);

#ifdef __KERNEL__
	/* This sequence is used set Hardware timestamp
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 */
	int (*set_hwts)(void *ops, struct ifreq *req);

	/* This sequence is used get Hardware timestamp
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 */
	int (*get_hwts)(void *ops, struct ifreq *req);

	/* This sequence is used for Rx Hardware timestamp operations
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	-1: Initialize MAC Error
	 */
	int (*do_rx_hwts)(void *ops, struct sk_buff *skb);

	/* This sequence is used for Tx Hardware timestamp operations
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	-1: Initialize MAC Error
	 */
	int (*do_tx_hwts)(void *ops, struct sk_buff *skb);

	/* This sequence is get Timestamp info
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	-1: Initialize MAC Error
	 */
	int (*mac_get_ts_info)(void *ops, struct ethtool_ts_info *ts_info);
#endif

	/* This sequence is to do soft restart of Xgmac
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	-1:	Initialize MAC Error
	 */
	void (*soft_restart)(void *ops);

	/* This sequence is used to retrieve XG/L-MAC counter
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]OUT:	mac_rmon
	 * return	OUT:	-1: Exit MAC Error
	 */
	int (*rmon_get)(void *ops, struct mac_rmon *rmon);

	/* This sequence is used to clear XG/L-MAC counter
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT:	-1: Exit MAC Error
	 */
	int (*rmon_clr)(void *ops);

	/* This sequence is used Initialize MAC
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	-1: Initialize MAC Error
	 */
	int (*init)(void *ops);

	/* This sequence is used Exit MAC
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	-1:	Exit MAC Error
	 */
	int (*exit)(void *ops);

	/* This sequence is used for Xgmac Cli implementation
	 * param[in/out]IN:	GSW_MAC_Cli_t - Argument list.
	 * return	OUT	-1: Exit MAC Error
	 */
	int (*xgmac_cli)(GSW_MAC_Cli_t *arg);

	/* This sequence is used for Lmac Cli implementation
	 * param[in/out]IN:	GSW_MAC_Cli_t - Argument list.
	 * return	OUT	-1: Exit MAC Error
	 */
	int (*lmac_cli)(GSW_MAC_Cli_t *arg);

	/* This sequence is used for Reading XGMAC register
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	u32 -   Register Offset.
	 * param[in/out]OUT:	u32 -   Register Value.
	 * return	OUT	u32 -	Error code or 0 when successful.
	 */
	int (*xgmac_reg_rd)(void *ops, u32 off, u32 *pval);

	/* This sequence is used for Writing XGMAC register
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	u32 -   Register Offset.
	 * param[in/out]IN:	u32 -   Register Value.
	 */
	int (*xgmac_reg_wr)(void *ops, u32 off, u32 val);

	/* This sequence is used for Reading LMAC register
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	u32 -   Register Offset.
	 * return	OUT	u32 -	Register Value
	 */
	int (*lmac_reg_rd)(void *ops, u32 off);

	/* This sequence is used for Writing LMAC register
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	u32 -   Register Offset.
	 * param[in/out]IN:	u32 -   Register Value.
	 */
	int (*lmac_reg_wr)(void *ops, u32 off, u32 val);

	/* This sequence is used for Registering IRQ Callback for a event
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	GSW_Irq_Op_t -   IRQ event info.
	 * return	OUT	int -	Success/Fail
	 */
	int (*IRQ_Register)(void *ops, GSW_Irq_Op_t *irq_op);

	/* This sequence is used for UnRegistering IRQ Callback for a event
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	GSW_Irq_Op_t -   IRQ event info.
	 * return	OUT	int -	Success/Fail
	 */
	int (*IRQ_UnRegister)(void *ops, GSW_Irq_Op_t *irq_op);

	/* This sequence is used for Enabling IRQ for a event
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	GSW_Irq_Op_t -   IRQ event info.
	 * return	OUT	int -	Success/Fail
	 */
	int (*IRQ_Enable)(void *ops, GSW_Irq_Op_t *irq_op);

	/* This sequence is used for Disabling IRQ for a event
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	GSW_Irq_Op_t -   IRQ event info.
	 * return	OUT	int -	Success/Fail
	 */
	int (*IRQ_Disable)(void *ops, GSW_Irq_Op_t *irq_op);

	/* This sequence is used for Enabling MAC Interrupt
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	int -	Success/Fail
	 */
	int (*mac_int_en)(void *ops);

	/* This sequence is used for Disabling MAC Interrupt
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	int -	Success/Fail
	 */
	int (*mac_int_dis)(void *ops);

	/* This sequence is used for Configuring Mac operation
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	MAC_OP_CFG - operation to perform
	 * return	OUT	int -	Success/Fail
	 */
	int (*mac_op_cfg)(void *ops, MAC_OPER_CFG cfg);

   /* This sequence is used for MACsec offload MAC support check
    * param[in/out]IN:  ops - MAC ops Struct registered for MAC 0/1/2.
    * param[out]OUT: eip160_pdev - EIP160 Meta Data
    * return   OUT   int - Success/Fail
    */
	int (*mac_offload_mac_check)(void *ops, struct mac_eip160_ops *e160);

	/* This sequence is used for MACsec Preparation
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * param[out]OUT:	eip160_pdev - EIP160 Meta Data
	 * return	OUT	int -	Success/Fail
	 */
	int (*mac_eip160_prepare)(void *ops, struct mac_eip160_ops *e160);
	/* This sequence is used for MACsec Un-preparation
	 * param[in/out]IN:	ops -	MAC ops Struct registered for MAC 0/1/2.
	 * return	OUT	int -	Success/Fail
	 */
	int (*mac_eip160_unprepare)(void *ops, struct mac_eip160_ops *e160);
	/* Set MAC mode and configure MAC interface
	 */
	int (*mac_config_mode)(void *ops, phy_interface_t mode);

	/* This function is used to enable/disable MAC receiver
	 * Some phys takes time for signal to be stable after link up. To avoid
	 * unstable signal causing MAC errors, providing this API for application
	 * to disable MAC RX until the signal is stable.
	 * param[in/out]IN:	ops	MAC ops Struct registered for MAC 0/1/2.
	 * param[in/out]IN:	enable	u32 - Enable/Disable MAC receiver
	 * return	OUT	int	MAC receiver enable Error
	 */
	int (*mac_receiver_enable)(void *ops, u32 enable);
};

#endif
