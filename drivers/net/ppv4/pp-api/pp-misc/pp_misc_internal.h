/*
 * Copyright (C) 2020-2021 MaxLinear, Inc.
 * Copyright (C) 2018-2020 Intel Corporation
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
 * Description: PP Misc Internal Definitions
 */

#ifndef __PP_MISC_INTERNAL_H__
#define __PP_MISC_INTERNAL_H__

#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/pp_api.h>
#include "pp_dev.h"
#include "pp_misc.h"

/**
 * @brief Add function name and line number for all pr_* prints
 */
#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "[PP_MISC]:%s:%d: " fmt, __func__, __LINE__
#endif

/**
 * @brief get the module statistics from database
 * @param stats port manager module statistics
 * @return s32 0 for success, non-zero otherwise
 */
s32 misc_stats_get(struct misc_stats *stats);

/**
 * @brief get session statistics for brief command
 * @param stats session statistics
 * @param num_stats number of session statistics
 * @param data data
 * @return s32 0 for success, non-zero otherwise
 */
s32 pp_brief_stats_get(void *stats, u32 num_stats, void *data);

/**
 * @brief Calculate packets and bytes diff for opened sessions
 * @param pre session statistics before sleep
 * @param num_pre number of session statistics
 * @param post session statistics after sleep
 * @param num_post number of session statistics for post
 * @param delta session statistics holding the delta between pre and post
 * @param num_delta number of session statistics for delta
 * @param data data
 * @return s32 0 for success, non-zero otherwise
 */
s32 pp_brief_stats_diff(void *pre, u32 num_pre, void *post, u32 num_post,
			 void *delta, u32 num_delta, void *data);

/**
 * @brief Show session statistics for brief command
 * @param buf buffer for prints
 * @param stats buffer length including trailing '\0'
 * @param n number of characters written to the 'buf' excluding
 *          trailing '\0'
 * @param stats session statistics
 * @param num_stats umber of session statistics
 * @param data session statistics
 * @return s32 0 for success, non-zero otherwise
 */
s32 pp_brief_stats_show(char *buf, size_t sz, size_t *n, void *stats,
			     u32 num_stats, void *data);

/**
 * @brief Set max session statistics for brief command
 * @param cnt max session count
 * @return s32 0 for success, non-zero otherwise
 */
void pp_misc_set_brief_cnt(u32 cnt);

/**
 * @brief Reset stats
 * @return s32 0 for success, non-zero otherwise
 */
s32 misc_stats_reset(void);

/**
 * @brief misc sysfs init
 * @param parent parent folder to create all files in
 * @return s32
 */
s32 pp_misc_sysfs_init(struct kobject *parent);

/**
 * @brief misc sysfs cleanup
 * @return s32
 */
void pp_misc_sysfs_clean(void);

/**
 * @brief get fw lld context
 * @param sf_id service flow id
 * @param ctx context
 * @return s32 0 on success
 */
s32 pp_misc_fw_lld_ctx_get(u8 sf_id, u8* ctx);

#ifdef CONFIG_DEBUG_FS
/**
 * @brief misc debug init
 * @param ppv4_base ppv4 registers base address
 * @return s32
 */
s32 pp_misc_dbg_init(struct pp_misc_init_param *init_param);

/**
 * @brief misc debug cleanup
 * @return s32
 */
s32 pp_misc_dbg_clean(void);

/**
 * @brief Get the status of ignore updates
 * @return 1 for ignoring, 0 for listen to notification
 */
u32 ignore_clk_updates_get(void);

/**
 * @brief Set whether to listen or ignore clk freq updates
 * @param ignore_updates new configuration for ignoring or not
 * @return none
 */
void ignore_clk_updates_set(u32 ignore_updates);

/**
 * @brief Get whitelist group id
 * @return s32 group id or error code if occur
 */
s32 pp_misc_whitelist_grp_id_get(void);
#else
static inline s32 pp_misc_dbg_init(struct pp_misc_init_param *init_param)
{
	return 0;
}

static inline s32 pp_misc_dbg_clean(void)
{
	return 0;
}
#endif /* CONFIG_DEBUG_FS */

#endif /* __PP_MISC_INTERNAL_H__ */
