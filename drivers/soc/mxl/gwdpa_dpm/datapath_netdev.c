// SPDX-License-Identifier: GPL-2.0
/*****************************************************************************
 * Copyright (c) 2025, MaxLinear, Inc.
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.

*******************************************************************************/

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/if_arp.h>
#include <linux/if_link.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>

#include <linux/soc/mxl/datapath_api.h>

static void dummy_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *info)
{
	strlcpy(info->driver, "dp_dummy_dev", sizeof(info->driver));
}

static const struct ethtool_ops dummy_ethtool_ops = {
	 .get_drvinfo = dummy_get_drvinfo,
};

static const struct net_device_ops dummy_netdev_ops = {
	.ndo_setup_tc = dp_ndo_setup_tc,
};

static void setup_dummy(struct net_device *dev)
{
	ether_setup(dev);

	dev->netdev_ops = &dummy_netdev_ops;
	dev->ethtool_ops = &dummy_ethtool_ops;

	dev->hw_features |= NETIF_F_HW_TC;
	dev->features |= NETIF_F_HW_TC;

	dev->operstate = IF_OPER_UP;

	netif_carrier_on(dev);

	eth_hw_addr_random(dev);
}

struct net_device *dp_create_netdev(const char *name)
{
	/* Note: cannot use alloc_netdev if need support multiple tx queue */
	struct net_device *netdev = alloc_netdev_mqs(0, name, NET_NAME_ENUM,
						     setup_dummy, 8, 1);

	if (!netdev)
		return NULL;
	if (register_netdev(netdev)) {
		free_netdev(netdev);
		return NULL;
	}
	netif_set_real_num_tx_queues(netdev, 8);
	netif_tx_start_all_queues(netdev);
	return netdev;
}

