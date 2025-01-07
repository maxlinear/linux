/*
 * pp_qos_common.h
 * Description: Packet Processor QoS Driver
 *
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2017-2019 Intel Corporation
 */
#ifndef PP_QOS_COMMON_H
#define PP_QOS_COMMON_H

#ifdef __KERNEL__
#include <linux/bitops.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/stringify.h>
#else
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "list.h"
#endif

/* #define PP_QOS_DISABLE_CMDQ */
/* #define PRINT_CREATE_CMD */
/* Additional 4 bytes for the validation signature */
#define PPV4_QOS_STAT_SIZE           \
	(max(sizeof(struct qm_info), \
	sizeof(struct system_stats_s)) + 4)
/* Logs */
#define QOS_LOG_CRIT(format, arg...)      pr_crit(format, ##arg)
#define QOS_LOG_ERR(format, arg...)       pr_err(format, ##arg)
#define QOS_LOG_INFO(format, arg...)      pr_info(format, ##arg)
#define QOS_LOG_DEBUG(format, arg...)     pr_debug(format, ##arg)
#define QOS_LOG_API_DEBUG(format, arg...) pr_debug("[API] " format, ##arg)
#define QOS_LOG_GET_API_DEBUG(format, arg...) \
	pr_debug("[GET API] " format, ##arg)
#define QOS_LOG_API_STR_MAX               (1024)

/* Locking */
#define LOCK spinlock_t
#define QOS_LOCK_INIT(qdev)      spin_lock_init(&qdev->lock)
#define QOS_LOCK(qdev)           spin_lock_bh(&qdev->lock)
#define QOS_UNLOCK(qdev)         spin_unlock_bh(&qdev->lock)
#define QOS_SPIN_IS_LOCKED(qdev) spin_is_locked(&qdev->lock)

#define PPV4_QOS_AQM_BUF_OFFSET        (0x7FC0)
#define PPV4_QOS_AQM_BUF_SIZE          (0x40)

#define DUMP_LOG_ON_FAILURE

#endif /* PP_QOS_COMMON_H */
