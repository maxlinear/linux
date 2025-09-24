// SPDX-License-Identifier: GPL-2.0
/******************************************************************************
 * Copyright (c) 2025 MaxLinear, Inc.
 *
 * TODO: This file will be removed once the permanent solution for CPU ingress
 * QoS is implemented.
 *
 *****************************************************************************/
enum omci_hostif_action {
	OMCI_HOSTIF_ADD,
	OMCI_HOSTIF_DEL,
};

int dp_omci_update_hostif(struct net_device *dev, int action);
