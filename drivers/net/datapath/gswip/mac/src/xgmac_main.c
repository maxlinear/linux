/******************************************************************************
 *                Copyright (C) 2020-2022 MaxLinear, Inc.
 *                Copyright (c) 2016-2020 Intel Corporation
 *
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.
 *
 ******************************************************************************/
#include <xgmac.h>
#include <gswss_mac_api.h>
#include <xgmac_mdio.h>
#include <mac_cfg.h>
#include <lmac_api.h>

u32 wakeup_filter_config[] = {
	/* for filter 0 CRC is computed on 0 - 7 bytes from offset */
	0x000000ff,
	/* for filter 1 CRC is computed on 0 - 7 bytes from offset */
	0x000000ff,
	/* for filter 2 CRC is computed on 0 - 7 bytes from offset */
	0x000000ff,
	/* for filter 3 CRC is computed on 0 - 7 bytes from offset */
	0x000000ff,
	/* filter 0, 1 independently enabled and would apply for
	 * unicast packet only filter 3, 2 combined as,
	 * "Filter-3 pattern AND NOT Filter-2 pattern"
	 */
	0x01010101,
	/* filter 3, 2, 1 and 0 offset is 14, 22, 30, 38 bytes
	 * from start
	 */
	0x0e161e26,
	/* pattern for filter 1 and 0, "0x55", "11", repeated 8 times */
	0x0B400B40,
	/* pattern for filter 3 and 2, "0x44", "33", repeated 8 times */
	0x0B400B40,
};

int populate_filter_frames(void *pdev)
{
	int i;
	struct mac_prv_data *pdata = GET_MAC_PDATA(pdev);

	for (i = 0; i < 8; i++)
		pdata->rwk_filter_values[i] = wakeup_filter_config[i];

	pdata->rwk_filter_length = 8;

	return 0;
}

int xgmac_wr_reg(void *pdev, u32 reg_off, u32 reg_val)
{
	struct mac_prv_data *pdata = GET_MAC_PDATA(pdev);

	if (reg_off >= 0x4000)
		return -EFAULT;

	reg_off &= 0x3FFC;
	XGMAC_RGWR(pdata, reg_off, reg_val);

	return 0;
}

int xgmac_rd_reg(void *pdev, u32 reg_off, u32 *pval)
{
	struct mac_prv_data *pdata = GET_MAC_PDATA(pdev);

	if (!pval)
		return -EINVAL;

	if (reg_off >= 0x4000)
		return -EFAULT;

	reg_off &= 0x3FFC;
	*pval = XGMAC_RGRD(pdata, reg_off);

	return 0;
}


u8 mac_addr[6] = {0x00, 0x00, 0x94, 0x00, 0x00, 0x08};

void xgmac_init_pdata(struct mac_prv_data *pdata, int idx)
{
#ifdef __KERNEL__
	memset(pdata, 0, sizeof(struct mac_prv_data));
	pdata->mac_idx = idx + MAC_2;
#else

	if (idx == -1)
		pdata->mac_idx = (pdata - &prv_data[0]) + MAC_2;
	else
		pdata->mac_idx = idx + MAC_2;

#endif

	pdata->xgmac_ctrl_reg = XGMAC_CTRL_REG(pdata->mac_idx);
	pdata->xgmac_data1_reg = XGMAC_DATA1_REG(pdata->mac_idx);
	pdata->xgmac_data0_reg = XGMAC_DATA0_REG(pdata->mac_idx);

	pdata->tx_q_count		= 1;
	pdata->rx_q_count		= 1;
	pdata->tx_sf_mode		= 0;
	pdata->tx_threshold		= MTL_TX_THRESHOLD_64;
	pdata->rx_sf_mode		= 0;
	pdata->rx_threshold		= MTL_RX_THRESHOLD_64;

	mac_addr[5] = pdata->mac_idx + 1;
	memcpy(pdata->mac_addr,	mac_addr, 6);

	pdata->tstamp_addend		= 0;
	pdata->tx_tstamp		= 0;
	pdata->phy_speed		= SPEED_XGMAC_10G;
	pdata->usxgmii			= false;
	pdata->mode			= PHY_INTERFACE_MODE_NA;
	pdata->promisc_mode		= 1;
	pdata->all_mcast_mode		= 1;
	pdata->rfa			= 2;
	pdata->rfd			= 4;
	pdata->tx_mtl_alg		= MTL_ETSALG_WRR;
	pdata->rx_mtl_alg		= MTL_RAA_SP;
	pdata->mtu			= FALCON_MAX_MTU;
	pdata->pause_time		= 0xFFFF;
	pdata->rx_checksum_offload	= 1;
	pdata->pause_frm_enable		= 1;
	pdata->rmon_reset		= 1;
	pdata->loopback			= 0;
	pdata->eee_enable		= 1;
	pdata->lst			= 1000;
	pdata->twt			= 80;
	pdata->lpitxa			= 0;
	pdata->crc_strip		= 0;
	pdata->crc_strip_type		= 0;
	pdata->padcrc_strip		= 0;
	pdata->rmon_reset		= 1;
	pdata->fup			= 1;
	pdata->fef			= 1;
	pdata->mac_en			= 1;
	pdata->ipg			= 0;
	pdata->enable_mac_int		= XGMAC_ALL_EVNT;
	pdata->enable_mtl_int		= MASK(MTL_Q_IER, TXUIE) |
					  MASK(MTL_Q_IER, ABPSIE) |
					  MASK(MTL_Q_IER, RXOIE);
	/* Calc as (2^32 * 250Mhz)/ 500Mhz */
	pdata->def_addend		= 0x80000000;
	pdata->sec			= 0;
	pdata->nsec			= 0;
	pdata->ptp_clk			= PTP_CLK;
	pdata->one_nsec_accuracy	= 1;
	pdata->systime_initialized      = 0;
#if defined(PC_UTILITY) || defined(CHIPTEST)
	pdata->lmac_addr_base		= LEGACY_MAC_BASE;
	pdata->ss_addr_base		= adap_priv_data.ss_addr_base;
	pdata->max_mac			= MAX_MAC;
	//Needed for continuous run of PMAC Time stamp and OAM time stamp tests in SWAT
	memset(&pdata->ptp_flgs, 0, sizeof(struct ptp_flags));

#endif
}

#if defined(PC_UTILITY) || defined(CHIPTEST)
struct mac_ops *gsw_get_mac_ops(u32 devid, u32 mac_idx)
{
	return &prv_data[mac_idx].ops;
}

u32 gsw_get_mac_subifcnt(u32 devid)
{
	return MAX_MAC;
}
#endif
