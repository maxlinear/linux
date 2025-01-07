/*
 * Copyright (C) 2020-2024 MaxLinear, Inc.
 * Copyright (C) 2018-2020 Intel Corporation
 *
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
 * Description: PP Device Driver
 */

#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/pp_api.h>
#include <linux/pp_qos_api.h>
#include "pp_dev.h"
#include "pp_qos_common.h"
#include "pp_common.h"

/**
 * @brief Add function name and line number for all pr_* prints
 */
#ifdef pr_fmt
#undef pr_fmt
#define pr_fmt(fmt) "[PP_DTS]:%s:%d: " fmt, __func__, __LINE__
#endif

/**
 * @brief Debug print current DT resource
 * @param dev sub-device
 * @param name resource name
 * @param r resource
 */
static void print_resource(struct device *dev,
			   const char *name,
			   struct resource *r)
{
	dev_dbg(dev, "%s memory resource: start(0x%08zX), size(%zu)\n", name,
		(size_t)(uintptr_t)r->start,
		(size_t)(uintptr_t)resource_size(r));
}

/**
 * @brief get u32 parameter from the device node
 * @param pdev sub-device
 * @param dn device node
 * @param name property name
 * @param dst returned u32
 * @return s32 0 on success, non-zero value otherwise
 */
static inline s32 __dts_u32_param_get(struct platform_device *pdev,
				      struct device_node *dn,
				      char *name, u32 *dst)
{
	u32 val;
	s32 ret = 0;

	ret = of_property_read_u32(dn, name, &val);
	if (ret) {
		dev_dbg(&pdev->dev, "Couldn't get %s (%d)\n", name, ret);
		return ret;
	}
	dev_dbg(&pdev->dev, "%s = %d\n", name, val);
	if (dst)
		*dst = val;

	return ret;
}

/**
 * @brief map a resource and get the resource virt base address
 * @param pdev sub-device
 * @param name resource name
 * @param addr returned virt address
 * @return s32 0 on success, non-zero value otherwise
 */
static s32 __dts_virt_addr_remap(struct platform_device *pdev,
				 const char *name, u64 *addr,
				 enum pp_hw_module mod_id)
{
	struct pp_io_region region;
	struct resource	*res;
	void __iomem *virt;
	s32 err;

	if (unlikely(ptr_is_null(addr)))
		return -EINVAL;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (unlikely(!res)) {
		dev_err(&pdev->dev, "Could not get %s resource\n", name);
		return -ENODEV;
	}

	print_resource(&pdev->dev, name, res);

	virt = devm_ioremap_resource(&pdev->dev, res);
	if (unlikely(!virt)) {
		dev_err(&pdev->dev, "devm_ioremap_resource failed\n");
		return -ENOMEM;
	}

	region.virt   = virt;
	region.phys   = res->start;
	region.sz     = resource_size(res);
	region.mod_id = mod_id;
	err = pp_region_add(&region);
	if (unlikely(err))
		return err;

	*addr = (unsigned long)virt;

	return 0;
}

/**
 * @brief get resource physical address
 * @param pdev sub-device
 * @param name resource name
 * @param addr returned physical address
 * @return s32 0 on success, non-zero value otherwise
 */
static s32 __dts_phys_addr_get(struct platform_device *pdev,
			       const char *name, phys_addr_t *addr)
{
	struct resource	*res;

	if (unlikely(ptr_is_null(addr)))
		return -EINVAL;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, name);
	if (unlikely(!res)) {
		dev_err(&pdev->dev, "Could not get %s resource\n", name);
		return -ENODEV;
	}

	print_resource(&pdev->dev, name, res);

	if (unlikely(!res->start)) {
		dev_err(&pdev->dev, "Could not get %s resource start addr\n",
			name);
		return -ENODEV;
	}

	*addr = res->start;
	dev_dbg(&pdev->dev, "%s = 0x%llx\n", name, (u64)*addr);

	return 0;
}

static s32 __dts_sub_device_get(char *compatible, struct platform_device **pdev,
				struct device_node **np)
{
	*np = of_find_compatible_node(NULL, NULL, compatible);
	if (!*np) {
		pr_debug("null '%s' node\n", compatible);
		return -EINVAL;
	}

	*pdev = of_find_device_by_node(*np);
	if (!*pdev) {
		pr_err("null pdev\n");
		return -EINVAL;
	}

	return 0;
}

static s32 __dts_infra_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-infra", &pdev, &np))
		return 0;

	/* Fetch infra boot base address */
	ret = __dts_virt_addr_remap(pdev, "reg-boot",
				    &dts_cfg->infra_params.bootcfg_base,
				    HW_MOD_INFRA);
	if (unlikely(ret))
		return ret;

	/* Fetch infra clock control base address */
	ret = __dts_virt_addr_remap(pdev, "reg-clk",
				    &dts_cfg->infra_params.clk_ctrl_base,
				    HW_MOD_INFRA);

	if (!ret)
		dts_cfg->infra_params.valid = true;

	return ret;
}

#ifdef CONFIG_SOC_LGM
static s32 __dts_service_log_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-service-log", &pdev, &np))
		return 0;

	/* Fetch service log inga base address */
	ret = __dts_virt_addr_remap(pdev, "reg-inga",
				    &dts_cfg->infra_params.service_log_inga_base,
				    HW_MOD_INFRA);
	if (unlikely(ret))
		return ret;

	/* Fetch service log ingb base address */
	ret = __dts_virt_addr_remap(pdev, "reg-ingb",
				    &dts_cfg->infra_params.service_log_ingb_base,
				    HW_MOD_INFRA);
	if (unlikely(ret))
		return ret;

	/* Fetch service log qos base address */
	ret = __dts_virt_addr_remap(pdev, "reg-qos",
				    &dts_cfg->infra_params.service_log_qos_base,
				    HW_MOD_INFRA);

	if (!ret)
		dts_cfg->infra_params.is_srvs_log_valid = true;

	return ret;
}
#else
static inline s32 __dts_service_log_cfg(struct pp_dts_cfg *dts_cfg)
{
	return 0;
}
#endif /* CONFIG_SOC_LGM */

static s32 __dts_parser_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-parser", &pdev, &np))
		return 0;

	/* Fetch parser base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config",
				    &dts_cfg->parser_params.parser_base,
				    HW_MOD_PARSER);
	if (unlikely(ret))
		return ret;

	ret = __dts_u32_param_get(pdev, np, "parser-profile",
				  &dts_cfg->parser_params.parser_profile);

	if (!ret)
		dts_cfg->parser_params.valid = true;

	return ret;
}

static s32 __dts_rpb_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-rpb", &pdev, &np))
		return 0;

	/* Fetch rpb drop base address */
	ret = __dts_virt_addr_remap(pdev, "reg-drop",
				    &dts_cfg->rpb_params.rpb_drop_base, HW_MOD_RPB);
	if (unlikely(ret))
		return ret;

	/* Fetch rpb base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config",
				    &dts_cfg->rpb_params.rpb_base, HW_MOD_RPB);
	if (unlikely(ret))
		return ret;

	ret = __dts_u32_param_get(pdev, np, "profile",
				  &dts_cfg->rpb_params.rpb_profile);
	if (unlikely(ret))
		return ret;

	ret = __dts_u32_param_get(pdev, np, "memory-size",
				  &dts_cfg->rpb_params.mem_sz);
	if (unlikely(ret))
		return ret;

	ret = __dts_u32_param_get(pdev, np, "max-pending-packets",
				  &dts_cfg->rpb_params.num_pending_pkts);
	if (unlikely(ret))
		return ret;

	dts_cfg->rpb_params.valid = true;
	return 0;
}

static s32 __dts_port_dist_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-port-dist", &pdev, &np))
		return 0;

	/* Fetch port dist base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config",
				    &dts_cfg->port_dist_params.port_dist_base,
				    HW_MOD_PORT_DIST);

	if (!ret)
		dts_cfg->port_dist_params.valid = true;

	return ret;
}

static s32 __dts_chk_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-chk", &pdev, &np))
		return 0;

	/* Fetch checker base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config",
				    &dts_cfg->chk_param.chk_base,
				    HW_MOD_CHECKER);
	if (unlikely(ret))
		return ret;

	/* Fetch checker ram base address */
	ret = __dts_virt_addr_remap(pdev, "reg-ram",
				    &dts_cfg->chk_param.chk_ram_base,
				    HW_MOD_CHECKER);

	/* Fetch checker counters cache (DSI) address */
	ret = __dts_virt_addr_remap(pdev, "reg-cache",
				    &dts_cfg->chk_param.chk_cache_base,
				    HW_MOD_CHECKER);

	/* Fetch checker number of supported syncqs in HW */
	ret = __dts_u32_param_get(pdev, np, "num-hw-syncqs",
				  &dts_cfg->chk_param.num_syncqs);

	if (!ret)
		dts_cfg->chk_param.valid = true;

	return ret;
}

static s32 __dts_cls_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-cls", &pdev, &np))
		return 0;

	/* Fetch classifier base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config",
				    &dts_cfg->cls_param.cls_base,
				    HW_MOD_CLASSIFER);
	if (unlikely(ret))
		return ret;

	/* Fetch sessions cache (SI) base address */
	ret = __dts_virt_addr_remap(pdev, "reg-cache",
				    &dts_cfg->cls_param.cls_cache_base,
				    HW_MOD_CLASSIFER);
	if (unlikely(ret))
		return ret;

	/* Fetch checker number of lookup thread in HW */
	ret = __dts_u32_param_get(pdev, np, "num-lu-threads",
				  &dts_cfg->cls_param.num_lu_threads);
	if (unlikely(ret))
		return ret;

	dts_cfg->cls_param.valid = true;
	return 0;
}

static s32 __dts_mod_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-mod", &pdev, &np))
		return 0;

	/* Fetch modifier base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config",
				    &dts_cfg->mod_param.mod_base,
				    HW_MOD_MODIFIER);
	if (unlikely(ret))
		return ret;

	/* Fetch modifier ram base address */
	ret = __dts_virt_addr_remap(pdev, "reg-ram",
				    &dts_cfg->mod_param.mod_ram_base,
				    HW_MOD_MODIFIER);

	if (!ret)
		dts_cfg->mod_param.valid = true;

	return ret;
}

static s32 __dts_rxdma_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	struct device_node *buffer_size_node;
	u32 addr;
	s32 ret;

	if (__dts_sub_device_get("intel,ppv4-rxdma", &pdev, &np))
		return 0;

	ret = __dts_phys_addr_get(pdev, "qmgr-push",
				  &dts_cfg->rx_dma_params.qmgr_push_addr);
	if (unlikely(ret))
		return ret;

	ret = __dts_phys_addr_get(pdev, "qos-query",
				  &dts_cfg->rx_dma_params.qos_query_addr);
	if (unlikely(ret))
		return ret;

	ret = __dts_phys_addr_get(pdev, "wred-response",
				  &dts_cfg->rx_dma_params.wred_response_addr);
	if (unlikely(ret))
		return ret;

	ret = __dts_phys_addr_get(pdev, "bmgr-pop-push",
				  &dts_cfg->rx_dma_params.bmgr_pop_push_addr);
	if (unlikely(ret))
		return ret;

	ret = __dts_u32_param_get(pdev, np, "fsqm-buff-addr", &addr);
	if (unlikely(ret))
		return ret;
	dts_cfg->rx_dma_params.fsqm_buff_addr = addr;

	ret = __dts_u32_param_get(pdev, np, "fsqm-desc-addr", &addr);
	if (unlikely(ret))
		return ret;
	dts_cfg->rx_dma_params.fsqm_desc_addr = addr;

	ret = __dts_u32_param_get(pdev, np, "fsqm-sram-addr", &addr);
	if (unlikely(ret))
		return ret;
	dts_cfg->rx_dma_params.fsqm_sram_addr = addr;

	ret = __dts_u32_param_get(pdev, np, "fsqm-sram-sz",
				  &dts_cfg->rx_dma_params.fsqm_sram_sz);
	if (unlikely(ret))
		return ret;

	ret = __dts_u32_param_get(pdev, np, "fsqm-max-packet-size",
				  &dts_cfg->rx_dma_params.fsqm_max_pkt_sz);
	if (unlikely(ret))
		return ret;

	ret = __dts_u32_param_get(pdev, np, "fsqm-buff-offset",
				  &dts_cfg->rx_dma_params.fsqm_buff_offset);
	if (unlikely(ret))
		return ret;

	/* Fetch rx-dma buffer sizes */
	buffer_size_node = of_parse_phandle(np, "buff-size", 0);
	if (!buffer_size_node) {
		pr_err("Unable to get buffer size node\n");
		return -ENODEV;
	}
	ret = of_property_read_u32_array(buffer_size_node,
					 "mxl,bm-buff-pool-size",
					 dts_cfg->rx_dma_params.buffer_size,
					 PP_POLICY_PER_PORT);
	if (ret) {
		dev_err(&pdev->dev, "Couldn't get buff-size (%d)\n", ret);
		return -ENODEV;
	}

	dev_dbg(&pdev->dev, "rx-dma buffer sizes = [%u] [%u] [%u] [%u] [%u]\n",
		dts_cfg->rx_dma_params.buffer_size[0],
		dts_cfg->rx_dma_params.buffer_size[1],
		dts_cfg->rx_dma_params.buffer_size[2],
		dts_cfg->rx_dma_params.buffer_size[3],
		dts_cfg->rx_dma_params.buffer_size[4]);

	/* Fetch rx-dma base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config",
				    &dts_cfg->rx_dma_params.rx_dma_base,
				    HW_MOD_RXDMA);

	if (!ret)
		dts_cfg->rx_dma_params.valid = true;

	return ret;
}

static s32 __dts_bmgr_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	u32 val;
	s32 ret = 0;

	if (__dts_sub_device_get("intel,ppv4-bm", &pdev, &np))
		return 0;

	ret = __dts_u32_param_get(pdev, np, "max-policies", &val);
	if (unlikely(ret))
		return ret;
	dts_cfg->bmgr_params.max_policies = val;
	dts_cfg->bm_params.max_policies   = val;

	ret = __dts_u32_param_get(pdev, np, "max-groups", &val);
	if (unlikely(ret))
		return ret;
	dts_cfg->bmgr_params.max_groups = val;
	dts_cfg->bm_params.max_groups   = val;

	ret = __dts_u32_param_get(pdev, np, "max-pools", &val);
	if (unlikely(ret))
		return ret;
	dts_cfg->bmgr_params.max_pools = val;
	dts_cfg->bm_params.max_pools   = val;

	/* TODO: this name should be replaced */
	ret = __dts_u32_param_get(pdev, np, "max-pools-in-policy",
				  &dts_cfg->bmgr_params.max_pools_in_policy);
	if (unlikely(ret))
		return ret;

	ret = __dts_u32_param_get(pdev, np, "pool-pop-hw-en", &val);
	if (unlikely(ret))
		return ret;
	dts_cfg->bmgr_params.pool_pop_hw_en = !!val;

	ret = __dts_u32_param_get(pdev, np, "pcu-fifo-sz", &val);
	if (unlikely(ret))
		return ret;
	dts_cfg->bm_params.pcu_fifo_sz = val;

	/* Get buffer manager physical base  */
	ret = __dts_virt_addr_remap(pdev, "bmgr-pop-push",
				    &dts_cfg->bm_params.bm_pop_push_base,
				    HW_MOD_BMNGR);
	if (unlikely(ret))
		return ret;

	/**
	 * rpb hw modules requires buffer manager push & pop
	 * regiters base addr to be able to drop packets
	 * coming from memory ports
	 */
	dts_cfg->rpb_params.bm_phys_base = pp_virt_to_phys(
		(void *)(ulong)dts_cfg->bm_params.bm_pop_push_base);

	/* for FLM address handling differs */
	if (!IS_ENABLED(CONFIG_SOC_LGM)) {
		ret = __dts_u32_param_get(pdev, np, "bmgr-pop-push-hw", &val);
		if (unlikely(ret))
			return ret;

#ifndef CONFIG_X86_64
		dts_cfg->bm_params.bm_pop_push_phys = val;
#endif
	}

	/* Fetch bm base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config",
				    &dts_cfg->bm_params.bm_base, HW_MOD_BMNGR);
	if (unlikely(ret))
		return ret;

	/* Fetch bm ram base address */
	ret = __dts_virt_addr_remap(pdev, "reg-ram",
				    &dts_cfg->bm_params.bm_ram_base,
				    HW_MOD_BMNGR);

	if (likely(!ret)) {
		dts_cfg->bm_params.valid = true;
		dts_cfg->bmgr_params.valid = true;
	}

	return ret;
}

static s32 __dts_qos_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *np;
	u32 num_reserved_ports = 0;
	struct pp_qos_init_param *p = &dts_cfg->qos_params;
	u32 idx;
	s32 ret = 0;

	if (__dts_sub_device_get("intel,ppv4-qos", &pdev, &np))
		return 0;

	ret |= __dts_u32_param_get(pdev, np, "max-ports", &p->max_ports);
	ret |= __dts_u32_param_get(pdev, np, "max-queues", &p->max_queues);
	ret |= __dts_u32_param_get(pdev, np, "enhanced-wsp", &p->enhanced_wsp);
	ret |= __dts_u32_param_get(pdev, np, "wred-prioritize-pop",
				   &p->wred_prioritize_pop);
	ret |= __dts_u32_param_get(pdev, np, "wred-total-resources",
				   &p->wred_total_avail_resources);
	ret |= __dts_u32_param_get(pdev, np, "wred-p-const", &p->wred_p_const);
	ret |= __dts_u32_param_get(pdev, np, "wred-max-q-size",
				   &p->wred_max_q_size);
	ret |= __dts_u32_param_get(pdev, np, "num-reserved-ports",
				   &num_reserved_ports);
	ret |= __dts_u32_param_get(pdev, np, "num-reserved-queues",
				   &p->reserved_queues);
	ret |= __dts_u32_param_get(pdev, np, "fw-sec-data-stack-dccm",
				   &p->fw_sec_data_stack.is_in_dccm);
	ret |= __dts_u32_param_get(pdev, np, "fw-sec-data-stack-dccm-offset",
				   &p->fw_sec_data_stack.dccm_offset);
	ret |= __dts_u32_param_get(pdev, np, "fw-sec-data-stack-dccm-max-sz",
				   &p->fw_sec_data_stack.max_size);

	if (unlikely(ret))
		return ret;

	if (num_reserved_ports > PP_QOS_MAX_PORTS) {
		dev_err(&pdev->dev,
			"Num reserved ports %d exceeds maxmimum of %d\n",
			num_reserved_ports, PP_QOS_MAX_PORTS);
		return -EINVAL;
	}

	if (p->reserved_queues > p->max_queues) {
		dev_err(&pdev->dev,
			"Num reserved queues %d exceeds maxmimum of %d\n",
			p->reserved_queues, p->max_queues);
		return -EINVAL;
	}

	for (idx = 0 ; idx < num_reserved_ports ; idx++)
		dts_cfg->qos_params.reserved_ports[idx] = 1;

	/* Fetch buffer manager physical base */
	ret = __dts_phys_addr_get(pdev, "reg-bm-base",
				  &dts_cfg->qos_params.bm_base);
	if (unlikely(ret))
		return ret;

	/* Fetch qos uc base address */
	ret = __dts_virt_addr_remap(pdev, "reg-uc",
				    &dts_cfg->qos_params.qos_uc_base,
				    HW_MOD_QOS);
	if (unlikely(ret))
		return ret;

	/* Fetch qos wred base address */
	ret = __dts_virt_addr_remap(pdev, "reg-wred",
				    &dts_cfg->qos_params.qos_wred_base,
				    HW_MOD_QOS_WRED);
	if (unlikely(ret))
		return ret;

	/* Configure all qos uc base related addresses */
	dts_cfg->qos_params.dccm_db =
		(shared_db_t *)(ulong)dts_cfg->qos_params.qos_uc_base;

	/* Fetch qos misc uc address */
	ret = __dts_virt_addr_remap(pdev, "reg-misc-uc",
				    &dts_cfg->qos_params.qos_uc_misc,
				    HW_MOD_QOS);
	if (unlikely(ret))
		return ret;

	/* Fetch qos mbox uc address */
	ret = __dts_virt_addr_remap(pdev, "reg-mbx-to-uc",
				    &dts_cfg->qos_params.fwcom.mbx_to_uc,
				    HW_MOD_QOS);
	if (unlikely(ret))
		return ret;

	if (!ret)
		dts_cfg->qos_params.valid = true;

	return 0;
}

static s32 __dts_uc_cpu_cfg(struct platform_device *pdev,
			    struct device_node *dn,
			    struct uc_cpu_params *p)
{
	s32 ret = 0;

	/* get the uc cpus parameter */
	ret = __dts_u32_param_get(pdev, dn, "max-cpu", &p->max_cpu);
	if (unlikely(ret))
		return ret;

	ret = of_property_read_u32_array(dn, "profiles", p->cpu_prof,
					 ARRAY_SIZE(p->cpu_prof));
	if (unlikely(ret)) {
		dev_err(&pdev->dev, "Couldn't get uc profiles (%d)\n", ret);
		return -ENODEV;
	}

	return 0;
}

static s32 __dts_egr_uc_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *dn;
	s32 ret = 0;
	struct uc_egr_init_params *ucp = &dts_cfg->uc_params.egr;

	if (__dts_sub_device_get("intel,ppv4-egr-uc", &pdev, &dn))
		return 0;

	/* get the uc cpus parameters */
	ret = __dts_uc_cpu_cfg(pdev, dn, &ucp->cpus);
	if (unlikely(ret))
		goto done;

	/* Fetch uc base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config", &ucp->uc_base,
				    HW_MOD_EG_UC);
	if (unlikely(ret))
		goto done;

	/* Get buffer manager physical base */
	ret = __dts_phys_addr_get(pdev, "bm-base-addr", &ucp->bm_base);
	if (unlikely(ret))
		goto done;

	/* Get queue manager physical base */
	ret = __dts_phys_addr_get(pdev, "qm-base-addr", &ucp->qm_base);
	if (unlikely(ret))
		goto done;

	/* Get checker physical base */
	ret = __dts_phys_addr_get(pdev, "chk-base-addr", &ucp->chk_base);
	if (unlikely(ret))
		goto done;

	/* Get wred physical base */
	ret = __dts_phys_addr_get(pdev, "wred-base-addr", &ucp->wred_base);
	if (unlikely(ret))
		goto done;

	/* Get wred client physical base */
	ret = __dts_phys_addr_get(pdev, "client-base-addr", &ucp->client_base);
	if (unlikely(ret))
		goto done;

	/* Get TX-manager credit physical base */
	ret = __dts_phys_addr_get(pdev, "txm-cred-base-addr",
				  &ucp->txm_cred_base);
	if (unlikely(ret))
		goto done;

	/* Get classifier physical base */
	ret = __dts_phys_addr_get(pdev, "cls-base-addr", &ucp->cls_base);

	if (!ret)
		ucp->valid = true;

done:
	return ret;
}

static s32 __dts_ing_uc_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct platform_device *pdev;
	struct device_node *dn;
	s32 ret = 0;
	struct uc_ing_init_params *ucp = &dts_cfg->uc_params.ing;

	if (__dts_sub_device_get("intel,ppv4-ing-uc", &pdev, &dn))
		return 0;

	/* get the uc cpus parameters */
	ret = __dts_uc_cpu_cfg(pdev, dn, &ucp->cpus);
	if (unlikely(ret))
		goto done;

	/* Fetch uc base address */
	ret = __dts_virt_addr_remap(pdev, "reg-config", &ucp->uc_base,
				    HW_MOD_IN_UC);

	if (!ret)
		ucp->valid = true;

done:
	return ret;
}

static s32 dts_uc_cfg(struct pp_dts_cfg *dts_cfg)
{
	struct uc_init_params *uc = &dts_cfg->uc_params;
	int ret;

	uc->ppv4_base = dts_cfg->misc_params.ppv4_base;
	uc->ppv4_regs_sz = dts_cfg->misc_params.ppv4_regs_sz;

	ret = __dts_egr_uc_cfg(dts_cfg);
	if (ret)
		return ret;

	ret = __dts_ing_uc_cfg(dts_cfg);
	if (ret)
		return ret;

	return 0;
}

s32 __dts_cfg_get(struct pp_dts_cfg *dts_cfg)
{
	s32 ret = 0;

	ret = __dts_infra_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_service_log_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_parser_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_rpb_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_port_dist_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_chk_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_cls_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_mod_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_bmgr_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_rxdma_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = __dts_qos_cfg(dts_cfg);
	if (ret)
		goto error;

	ret = dts_uc_cfg(dts_cfg);
	if (ret)
		goto error;

	return 0;
error:
	return ret;
}

void pp_dts_cfg_dump(struct pp_dts_cfg *cfg)
{
#ifdef CONFIG_SOC_LGM
	int i;

	pr_debug("Clocks: ");
	for (i = 0; i < ARRAY_SIZE(cfg->pp_clocks); i++) {
		if (!cfg->pp_clocks[i])
			break;
		pr_debug("\t%s\n", cfg->pp_clocks[i]);
	}

	pr_debug("DMA NIOC size %u\n", cfg->dma_nioc_sz);
	pr_debug("DMA IOC size %u\n", cfg->dma_ioc_sz);
	pr_debug("Num sessions %u\n", cfg->cls_param.num_sessions);
	pr_debug("Num syncqs   %u\n", cfg->smgr_params.num_syncqs);
	pr_debug("Frag mode    %u\n", cfg->smgr_params.frag_mode);
	pr_debug("Accel Mode   %u\n", cfg->smgr_params.accl_mode);
	pr_debug("Bootcfg Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->infra_params.bootcfg_base));
	pr_debug("Clock Ctrl Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->infra_params.clk_ctrl_base));
	pr_debug("Service_log inga Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->infra_params.service_log_inga_base));
	pr_debug("Service_log ingb Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->infra_params.service_log_ingb_base));
	pr_debug("Service_log qos Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->infra_params.service_log_qos_base));
	pr_debug("RPB Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->rpb_params.rpb_base));
	pr_debug("RPB Drop Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->rpb_params.rpb_drop_base));
	pr_debug("RPB BM Phys Base 0x%llx\n", (u64)cfg->rpb_params.bm_phys_base);
	pr_debug("RPB Profile %u\n", cfg->rpb_params.rpb_profile);
	pr_debug("RPB Memory size %u bytes\n", cfg->rpb_params.mem_sz);
	pr_debug("RPB Max Pending Packets %u\n",
		cfg->rpb_params.num_pending_pkts);
	pr_debug("Parser Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->parser_params.parser_base));
	pr_debug("Port-Dist Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->port_dist_params.port_dist_base));
	pr_debug("Cls Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->cls_param.cls_base));
	pr_debug("Cls number of lookup threads %u\n",
		cfg->cls_param.num_lu_threads);
	pr_debug("Cls Session Cache Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->cls_param.cls_cache_base));
	pr_debug("Chk Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->chk_param.chk_base));
	pr_debug("Chk Ram Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->chk_param.chk_ram_base));
	pr_debug("Chk max-hw-syncqs %u\n", cfg->chk_param.num_syncqs);
	pr_debug("Counters Cache Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->chk_param.chk_cache_base));
	pr_debug("Mod Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->mod_param.mod_base));
	pr_debug("Mod Ram Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->mod_param.mod_ram_base));
	pr_debug("RxDma Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->rx_dma_params.rx_dma_base));
	pr_debug("qmgr_push_addr 0x%llx\n", cfg->rx_dma_params.qmgr_push_addr);
	pr_debug("qos_query_addr 0x%llx\n",
		cfg->rx_dma_params.qos_query_addr);
	pr_debug("wred_response_addr 0x%llx\n",
		cfg->rx_dma_params.wred_response_addr);
	pr_debug("bmgr_pop_push_addr 0x%llx\n",
		cfg->rx_dma_params.bmgr_pop_push_addr);
	pr_debug("fsqm buff addr 0x%llx\n",
		cfg->rx_dma_params.fsqm_buff_addr);
	pr_debug("fsqm desc addr 0x%llx\n",
		cfg->rx_dma_params.fsqm_desc_addr);
	pr_debug("fsqm buff offset %#x\n",
		cfg->rx_dma_params.fsqm_buff_offset);
	pr_debug("fsqm max packet size %u\n",
		cfg->rx_dma_params.fsqm_max_pkt_sz);
	pr_debug("RxDma Buffer-size [%u][%u][%u][%u][%u]\n",
		cfg->rx_dma_params.buffer_size[0],
		cfg->rx_dma_params.buffer_size[1],
		cfg->rx_dma_params.buffer_size[2],
		cfg->rx_dma_params.buffer_size[3],
		cfg->rx_dma_params.buffer_size[4]);
	pr_debug("Bmgr Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->bm_params.bm_base));
	pr_debug("Bmgr Ram Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->bm_params.bm_ram_base));
	pr_debug("Bmgr POP/PUSH Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->bm_params.bm_pop_push_base));
	pr_debug("Bmgr pools %u\n", cfg->bm_params.max_pools);
	pr_debug("Bmgr policies %u\n", cfg->bm_params.max_policies);
	pr_debug("Bmgr groups %u\n", cfg->bm_params.max_groups);
	pr_debug("Qos uC Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->qos_params.qos_uc_base));
	pr_debug("Qos WRED Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->qos_params.qos_wred_base));
	pr_debug("Qos CoDel BM Base 0x%llx\n", cfg->qos_params.bm_base);
	pr_debug("Qos ports %u\n", cfg->qos_params.max_ports);
	pr_debug("Qos queues %u\n", cfg->qos_params.max_queues);
	pr_debug("Qos Wred: resources %u. p_const %u. max_q_size %u.\n",
		cfg->qos_params.wred_total_avail_resources,
		cfg->qos_params.wred_p_const,
		cfg->qos_params.wred_max_q_size);
	pr_debug("Egress uC Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->uc_params.egr.uc_base));
	pr_debug("Egress uC max-cpus %u\n", cfg->uc_params.egr.cpus.max_cpu);
	pr_debug("Egress uC cpu0 profile %u\n",
		cfg->uc_params.egr.cpus.cpu_prof[0]);
	pr_debug("Egress uC cpu1 profile %u\n",
		cfg->uc_params.egr.cpus.cpu_prof[1]);
	pr_debug("Egress uC cpu2 profile %u\n",
		cfg->uc_params.egr.cpus.cpu_prof[2]);
	pr_debug("Egress uC cpu3 profile %u\n",
		cfg->uc_params.egr.cpus.cpu_prof[3]);
	pr_debug("Egress uC BM Base 0x%llx\n", cfg->uc_params.egr.bm_base);
	pr_debug("Egress uC QM Base 0x%llx\n", cfg->uc_params.egr.qm_base);
	pr_debug("Egress uC CHK Base 0x%llx\n", cfg->uc_params.egr.chk_base);
	pr_debug("Egress uC WRED Base 0x%llx\n", cfg->uc_params.egr.wred_base);
	pr_debug("Egress uC client Base 0x%llx\n",
		cfg->uc_params.egr.client_base);
	pr_debug("Egress uC TXM credit Base 0x%llx\n",
		cfg->uc_params.egr.txm_cred_base);
	pr_debug("Egress uC CLS Base 0x%llx\n", cfg->uc_params.egr.cls_base);
	pr_debug("Ingress uC Base 0x%llx\n",
		pp_virt_to_phys((void *)cfg->uc_params.ing.uc_base));
	pr_debug("Ingress uC max-cpus %u\n", cfg->uc_params.ing.cpus.max_cpu);
	pr_debug("Ingress uC cpu0 profile %u\n",
		cfg->uc_params.ing.cpus.cpu_prof[0]);
	pr_debug("Ingress uC cpu1 profile %u\n",
		cfg->uc_params.ing.cpus.cpu_prof[1]);
	pr_debug("Ingress uC cpu2 profile %u\n",
		cfg->uc_params.ing.cpus.cpu_prof[2]);
	pr_debug("Ingress uC cpu3 profile %u\n",
		cfg->uc_params.ing.cpus.cpu_prof[3]);
#endif /* CONFIG_SOC_LGM */
}

static s32 dts_get_clocks(struct platform_device *p, struct pp_dts_cfg *cfg)
{
	unsigned int i;
	int num_clks, ret;

	ret = of_property_count_strings(p->dev.of_node, "clock-names");
	if (ret <= 0) {
		dev_err(&p->dev, "Can't find PP clocks\n, ret = %d",
			ret);
		return ret;
	}

	num_clks = ret;
	for (i = 0; i < num_clks; i++) {
		ret = of_property_read_string_index(p->dev.of_node,
			"clock-names", i, &cfg->pp_clocks[i]);
		if (ret) {
			dev_err(&p->dev, "failed to get clock %d name", i);
			return ret;
		}
	}

	return 0;
}

s32 pp_dts_cfg_get(struct platform_device *pdev, struct pp_dts_cfg *cfg)
{
	u32 val;
	s32 ret;
	u32 ranges[3];

	ret = dts_get_clocks(pdev, cfg);
	if (ret)
		return ret;

	/* Fetch dma allocation size */
	ret = __dts_u32_param_get(pdev, pdev->dev.of_node, "dma_ioc_sz", &val);
	if (!ret)
		cfg->dma_ioc_sz = val;
	ret = __dts_u32_param_get(pdev, pdev->dev.of_node, "dma_nioc_sz", &val);
	if (!ret)
		cfg->dma_nioc_sz = val;

	/* Fetch number of supported sessions */
	ret = __dts_u32_param_get(pdev, pdev->dev.of_node, "num-sessions",
				  &val);
	if (!ret) {
		cfg->smgr_params.num_sessions = val;
		cfg->smgr_params.valid = true;
		cfg->cls_param.num_sessions = val;
		cfg->chk_param.num_sessions = val;
	}

	/* Fetch number of supported sync queues */
	ret = __dts_u32_param_get(pdev, pdev->dev.of_node, "num-syncqs", &val);
	if (ret) {
		cfg->smgr_params.valid = false;
	} else {
		cfg->smgr_params.num_syncqs = val;
		cfg->qos_params.num_syncqs  = val;
	}

	/* Fetch fragmentation mode */
	ret = __dts_u32_param_get(pdev, pdev->dev.of_node, "frag-mode", &val);
	if (ret)
		cfg->smgr_params.frag_mode = SMGR_FRAG_MODE_CHECK_MTU_IN_UC;
	else
		cfg->smgr_params.frag_mode = (enum smgr_frag_mode)val;

	/* Fetch acceleration mode */
	ret = __dts_u32_param_get(pdev, pdev->dev.of_node, "accel-mode", &val);
	if (ret)
		cfg->smgr_params.accl_mode = PP_ACCL_MODE_NORMAL;
	else
		cfg->smgr_params.accl_mode = (enum pp_accl_mode)val;

	/* Fetch PPv4 base address ranges */
	ret = of_property_read_u32_array(pdev->dev.of_node,
					 "ranges", ranges,
					 ARRAY_SIZE(ranges));
	if (unlikely(ret))
		return ret;

	cfg->misc_params.ppv4_base = ranges[1];
	cfg->misc_params.ppv4_regs_sz = ranges[2];
	pr_debug("PPv4 Base: 0x%lx (Sz: %u)\n",
		 (unsigned long)cfg->misc_params.ppv4_base,
		 cfg->misc_params.ppv4_regs_sz);
	cfg->misc_params.valid = true;

	ret = __dts_cfg_get(cfg);
	if (ret) {
		dev_err(&pdev->dev, "__dts_cfg_get failed (%d)\n", ret);
		return -ENODEV;
	}

	return ret;
}
