/******************************************************************************
 *                Copyright (C) 2020-2022 MaxLinear, Inc.
 *                Copyright (c) 2016-2020 Intel Corporation
 *
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.
 *
 ******************************************************************************/

#ifndef _MAC_CFG_
#define _MAC_CFG_

#include <xgmac_common.h>

#define RESET_OFF 	0
#define RESET_ON 	1

struct macsec_pvt_data {
	u32 devid;
	void __iomem *eip160_base;
	int irq_num;
};

/* MAC Interface API's */
int mac_config_loopback(void *pdev, u32 loopback);
int mac_config_ipg(void *pdev, u32 ipg);

int mac_set_mii_if(void *pdev, u32 mii_mode);
int mac_get_mii_if(void *pdev);

int mac_get_phymode(void *pdev, u8 *duplex, u8 *speed_mode, u8 *link_state);
int mac_set_speed(void *pdev, u32 phy_speed);
int mac_set_physpeed(void *pdev, u32 phy_speed);
int mac_get_speed(void *pdev);

int mac_set_duplex(void *pdev, u32 val);
int mac_get_duplex(void *pdev);

int mac_set_mtu(void *pdev, u32 mtu);
int mac_get_mtu(void *pdev);

int mac_set_pfsa(void *pdev, u8 *mac_addr, u32 mode);
int mac_get_pfsa(void *pdev, u8 *mac_addr, u32 *mode);

int mac_reset(void *pdev, u32 reset);

int mac_init(void *pdev);
int mac_exit(void *pdev);

int mac_set_flowctrl(void *pdev, u32 val);
int mac_get_flowctrl(void *pdev);

int mac_set_lpien(void *pdev, u32 enable, u32 lpi_waitg, u32 lpi_waitm);
int mac_get_lpien(void *pdev);

int mac_set_linksts(void *pdev, u32 linksts);
int mac_get_linksts(void *pdev);

int mac_set_fcs_gen(void *pdev, u32 val);
int mac_get_fcs_gen(void *pdev);

int mac_enable_ts(void *pdev);
int mac_disable_ts(void *pdev);

int mac_irq_enable(void *pdev, GSW_Irq_Op_t *irq);
int mac_irq_disable(void *pdev, GSW_Irq_Op_t *irq);
int mac_irq_register(void *pdev, GSW_Irq_Op_t *irq);
int mac_irq_unregister(void *pdev, GSW_Irq_Op_t *irq);

int mac_int_enable(void *pdev);
int mac_int_disable(void *pdev);

int mac_oper_cfg(void *pdev, MAC_OPER_CFG oper);
int xgmac_cfg_main(GSW_MAC_Cli_t *params);

int mac_eip160_prepare(void *pdev, struct mac_eip160_ops *eip160);
int mac_eip160_unprepare(void *pdev, struct mac_eip160_ops *eip160);

int mac_config_mode(void *pdev, phy_interface_t mode);
#endif

