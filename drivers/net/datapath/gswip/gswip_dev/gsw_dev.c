/******************************************************************************
 *                Copyright (C) 2020-2024 MaxLinear, Inc.
 *                Copyright (c) 2016-2020 Intel Corporation
 *
 *
 * For licensing information, see the file 'LICENSE' in the root folder of
 * this software module.
 *
 * This driver is the Parent GSWIP Subsystem driver, which Parses Device
 * Tree and starts multiple functional drivers for GSWIP.
 ******************************************************************************/

#include <net/switch_api/gsw_dev.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/reset.h>
#include <xgmac_common.h>
#include <xgmac.h>
#include <gswss_mac_api.h>
#include <gswss_api.h>
#include <gsw_flow_core.h>
#include <dt-bindings/net/mxl,lgm-cqm.h>

#define GRX500_MACH_NAME		"lantiq,xrx500"
#define PRX300_MACH_NAME		"intel,prx300"
#define LGM_MACH_NAME			"intel,lgm"
#define DEVID_STR			"intel,gsw-devid"
#define EXTERNAL_SWITCH_DEVID		"intel,gsw_ext-devid"
#define EXTERNAL_SWITCH_PHYID		"intel,gsw_ext-phyid"
#define EXTERNAL_SWITCH_BASEADDR	"intel,gsw_ext-baseaddr"
#define EXTERNAL_SWITCH_SGMIIBASEADDR   "intel,gsw_ext-sgmiibaseaddr"
#define GSW_GLOBAL_PCE_RULE_NUM         "intel,gsw-globalpce-rules"
#define GSW_AGING_TIMEOUT		"intel,gsw-aging-timeout"
#define GSW_EXTERNAL_OR_INTERNAL	"intel,gsw_ext-switch"
#define AUX0_TRIGGER			"intel,aux0-trigger"
#define AUX1_TRIGGER			"intel,aux1-trigger"
#define REF_TIME			"intel,ref-time"
#define PPS_SEL				"intel,pps-sel"

#define PRX300_STROBE			25000000
#define LGM_STROBE			125000000

#define LPID_EPG_MAP_VAL_SET 4

/* Structure for GSWIP Subsystem operations
 * used to start Sub-Functional Drivers
 */
struct gsw_cell {
	char                      name[128];
	int                       cell_id;
	u32                       device_id;
	u32                       prod_id;

	/* platform data passed to the sub devices drivers */
	void                      *platform_data;
	u32                       pdata_size;

	/* Driver data passed to the sub devices drivers */
	void                      *drv_data;
	u32                       drv_data_size;

	/*  Device Tree compatible string*/
	const char                *of_compatible;
	/* hese resources can be specified relative to the parent device.
	 * For accessing hardware you should use resources from the platform dev
	 */
	u32                       num_resources;
	struct resource           *resources;
	struct device_node        *of_node;
};

/* Globals */
#if !IS_ENABLED(CONFIG_OF)
/* Static declaration of Resources when device tree is not present */
static struct resource falc_macsec_res[] = {
	/* Macsec Ingress Instance 0 */
	{
		.start	= 0x18400000,
		.end	= 0x18420000,
		.flags	= IORESOURCE_MEM,
	},
	/* Macsec Egress Instance 1 */
	{
		.start	= 0x18420000,
		.end	= 0x18420000,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource falc_mac_res[] = {
	/* Adaption Resources */
	{
		.start	= 0x18440000,
		.end	= 0x18442000,
		.flags	= IORESOURCE_MEM,
	},
	/* LMAC resources */
	{
		.start	= 0x18442000,
		.end	= 0x18443000,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource falc_core_res[] = {
	/* GSW_CORE resources */
	{
		.start	= 0x18445000,
		.end	= 0x18444000,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource grx500_core_res_l[] = {
	/* GSW_CORE resources L */
	{
		.start	= 0x1C000000,
		.end	= 0x1C003000,
		.flags	= IORESOURCE_MEM,
	},
};

static struct resource grx500_core_res_r[] = {
	/* GSW_CORE resources R */
	{
		.start	= 0x1A000000,
		.end	= 0x1A003000,
		.flags	= IORESOURCE_MEM,
	},
};

/* Multi Functional Devices inside GSWIP Sub-System
 * for product PRX300
 */
static struct gsw_cell gsw_dev_cells[] = {
	/* MAC 0 */
	{
		.name = MAC_DEV_NAME,
		.cell_id = 0,
		.device_id = 0,
		.prod_id = PRX300,
		.of_compatible = "intel,gsw_mac",
		.num_resources = ARRAY_SIZE(falc_mac_res),
		.resources = &falc_mac_res;
	},
	/* MAC 1 */
	{
		.name = MAC_DEV_NAME,
		.cell_id = 1,
		.device_id = 0,
		.prod_id = PRX300,
		.of_compatible = "intel,gsw_mac",
		.num_resources = ARRAY_SIZE(falc_mac_res),
		.resources = &falc_mac_res;
	},
	/* MAC 2 */
	{
		.name = MAC_DEV_NAME,
		.cell_id = 2,
		.device_id = 0,
		.prod_id = PRX300,
		.of_compatible = "intel,gsw_mac",
		.num_resources = ARRAY_SIZE(falc_mac_res),
		.resources = &falc_mac_res;

	},
	/* GSW CORE L or PRX300 */
	{
		.name = CORE_DEV_NAME,
		.cell_id = 3,
		.device_id = 0,
		.prod_id = PRX300,
		.of_compatible = "intel,gsw_core",
		.num_resources = ARRAY_SIZE(falc_core_res),
		.resources = &falc_core_res;

	},

};
#endif

struct gswss gsw_dev[MAX_GSWDEV];

struct reset_control *gsw_reset_get(struct platform_device *pdev)
{
	struct reset_control *gsw_rst;

	gsw_rst = devm_reset_control_get(&pdev->dev, "gswip");

	if (IS_ERR(gsw_rst))
		return NULL;

	return gsw_rst;
}

static void gsw_rst(struct reset_control *gsw_rst)
{
	reset_control_assert(gsw_rst);
	/* Make sure one micro-second delay */
	udelay(1);
	reset_control_deassert(gsw_rst);
}

static int gsw_remove_data(u32 devid)
{
	int i = 0;
	struct mac_prv_data *mac_pdata = NULL;
	ethsw_api_dev_t *core_pdata = NULL;
	struct gsw_cell *cell;

	if (!gsw_dev[devid].cells)
		return 0;

	for (i = 0; i < gsw_dev[devid].num_devs; i++) {
		cell = &gsw_dev[devid].cells[i];
		if (cell->drv_data) {
			if (!strcmp(cell->name, MAC_DEV_NAME)) {
				mac_pdata = container_of(cell->drv_data,
							 struct mac_prv_data,
							 ops);
				cell->drv_data = NULL;
				kfree(mac_pdata);
			} else if (!strcmp(cell->name, CORE_DEV_NAME)) {
				core_pdata = container_of(cell->drv_data,
							  ethsw_api_dev_t,
							  ops);
				cell->drv_data = NULL;
				kfree(core_pdata->Global_Rule_BitMap.global_rule_idx_map);
				kfree(core_pdata);
			}
		}

		if (cell->platform_data) {
			kfree(cell->platform_data);
			cell->platform_data = NULL;
		}

		if (cell->resources) {
			kfree(cell->resources);
			cell->resources = NULL;
		}
	}

	if (gsw_dev[devid].cells_need_free) {
		kfree(gsw_dev[devid].cells);
		gsw_dev[devid].cells_need_free = 0;
	}
	gsw_dev[devid].cells = NULL;

	return 0;
}

static int gsw_add_macdev(struct gsw_cell *gsw_dev_cell, u32 devid)
{
	struct mac_prv_data *mac_pdata;
#ifdef CONFIG_OF
	struct resource irqres;
	u32 val;
#endif

	/* Allocate the private data for MAC */
	mac_pdata = kzalloc(sizeof(struct mac_prv_data), GFP_KERNEL);

	if (!mac_pdata)
		return -ENOMEM;

	/* Copy the initial values of MAC Private Data */
	xgmac_init_pdata(mac_pdata, (int)gsw_dev[devid].mac_subdevs_cnt);

#ifdef CONFIG_OF
	/* Retrieve external reference time flag */
	val = 0;
	of_property_read_u32(gsw_dev_cell->of_node, "ext-ref-time", &val);
	if (val)
		mac_pdata->ext_ref_time = true;

	mac_pdata->xgmac_err_pkt_fwd_fup =
		of_property_read_bool(gsw_dev_cell->of_node, "err-pkt-fwd-undersized");

	mac_pdata->xgmac_err_pkt_fwd_fef =
		of_property_read_bool(gsw_dev_cell->of_node, "err-pkt-fwd-errored");

	/* Retrieve ingress/egress time correction */
	of_property_read_s32(gsw_dev_cell->of_node, "ts-ig-corr",
			     &mac_pdata->ig_corr);
	of_property_read_s32(gsw_dev_cell->of_node, "ts-eg-corr",
			     &mac_pdata->eg_corr);

	of_irq_to_resource_table(gsw_dev_cell->of_node, &irqres, 1);

	mac_pdata->irq_num = irqres.start;

	/* Retrieve the speedset */
	of_property_read_u32(gsw_dev_cell->of_node, "speed",
			     &mac_pdata->phy_speed);

	/* Logic to Reset Adaption layer if RxFifo Error happens in MAC */
	of_property_read_u32(gsw_dev_cell->of_node, "rxfifo_err_thresh",
			     &mac_pdata->rxfifo_err.rxfifo_err_threshold);

	mac_pdata->usxgmii = of_property_read_bool(gsw_dev_cell->of_node,
						   "usxgmii");
	if (mac_pdata->usxgmii)
		mac_pdata->mode = PHY_INTERFACE_MODE_USXGMII;
#else

	if (gsw_dev[devid].prod_id == PRX300) {
		gsw_dev_cell->num_resources = ARRAY_SIZE(falc_mac_res);
		gsw_dev_cell->resources = falc_mac_res;
	}

#endif

	gsw_dev_cell->drv_data = (void *)(&mac_pdata->ops);
	gsw_dev_cell->drv_data_size = sizeof(mac_pdata->ops);

	return 0;
}

static int gsw_add_switchdev(struct gsw_cell *gsw_dev_cell, u32 devid)
{
	ethsw_api_dev_t *switch_pdata = NULL;
	int ret = 0;
	struct resource irqres;
	struct device_node *lpid_config;

	/* Allocate the private data for Switch */
	switch_pdata = kzalloc(sizeof(ethsw_api_dev_t), GFP_KERNEL);

	if (!switch_pdata)
		return -ENOMEM;

	/** Clear Switch Core Private Data */
	memset(switch_pdata, 0, sizeof(ethsw_api_dev_t));
	gsw_dev_cell->drv_data = (void *)(&switch_pdata->ops);
	gsw_dev_cell->drv_data_size = sizeof(switch_pdata->ops);

	of_irq_to_resource_table(gsw_dev_cell->of_node, &irqres, 1);
	switch_pdata->irq_num = irqres.start;
	switch_pdata->prod_id = gsw_dev[devid].prod_id;

	of_property_read_u32(gsw_dev_cell->of_node,
			     GSW_EXTERNAL_OR_INTERNAL,
			     &switch_pdata->ext_switch);

	if (switch_pdata->ext_switch) {
		of_property_read_u32(gsw_dev_cell->of_node,
				     EXTERNAL_SWITCH_PHYID,
				     &switch_pdata->ext_phyid);
		of_property_read_u32(gsw_dev_cell->of_node,
				     EXTERNAL_SWITCH_BASEADDR,
				     &switch_pdata->gswex_base);
		of_property_read_u32(gsw_dev_cell->of_node,
				     EXTERNAL_SWITCH_SGMIIBASEADDR,
				     &switch_pdata->gswex_sgmiibase);
	}

	of_property_read_u32(gsw_dev_cell->of_node, "gsw_mode",
			     &switch_pdata->gsw_mode);

	lpid_config = of_parse_phandle(gsw_dev_cell->of_node,
				       "mxl,lpid_config", 0);
	if (!IS_ERR_OR_NULL(lpid_config)) {
		u32 v4[LPID_EPG_MAP_VAL_SET * LPID_COUNT];
		u32 idx, num;
		u32 val;

		if (!of_property_read_u32(lpid_config, "mxl,port-mode", &val) &&
		    val == LPID_PORT_MODE1)
			switch_pdata->gsw_mode |= BIT(1);

		num = of_property_read_variable_u32_array(lpid_config,
							  "mxl,lpid-epg-map",
							  v4,
							  LPID_EPG_MAP_VAL_SET,
							  LPID_EPG_MAP_VAL_SET * LPID_COUNT);
		for (idx = 0; idx < LPID_COUNT; idx++) {
			if (idx < (num / LPID_EPG_MAP_VAL_SET)) {
				switch_pdata->lpid_epg_map[idx].lpid = v4[idx * LPID_EPG_MAP_VAL_SET];
				switch_pdata->lpid_epg_map[idx].epg = v4[idx * LPID_EPG_MAP_VAL_SET + 1];
				switch_pdata->lpid_epg_map[idx].txdma_ctrl = (u8)v4[idx * LPID_EPG_MAP_VAL_SET + 2];
				switch_pdata->lpid_epg_map[idx].txdma_chan = (u8)v4[idx * LPID_EPG_MAP_VAL_SET + 3];
			} else {
				switch_pdata->lpid_epg_map[idx].lpid = 0;
				switch_pdata->lpid_epg_map[idx].epg = 0;
				switch_pdata->lpid_epg_map[idx].txdma_ctrl = 0;
				switch_pdata->lpid_epg_map[idx].txdma_chan = 0;
			}
		}
	}

	/* Get the Number of Common/Global TFLOW Rules */
	of_property_read_u32(gsw_dev_cell->of_node, GSW_GLOBAL_PCE_RULE_NUM,
			     &switch_pdata->num_of_global_rules);
	/* Get default MAC table aging time */
	of_property_read_u32(gsw_dev_cell->of_node, GSW_AGING_TIMEOUT,
			     &switch_pdata->aging_timeout);
	of_property_read_u32(gsw_dev_cell->of_node, DEVID_STR,
			     &switch_pdata->devid);
	of_property_read_u32(gsw_dev_cell->of_node, "intel,eg-sptag",
			     &switch_pdata->eg_sptag);
	of_property_read_u32(gsw_dev_cell->of_node,
			     "mxl,pmac-fcs-enable",
			     &switch_pdata->pmac_fcs_enable);
#ifndef CONFIG_OF

	if (gsw_dev[devid].prod_id == GRX500) {
		if (gsw_dev_cell->device_id == 0) {
			gsw_dev_cell->num_resources =
				ARRAY_SIZE(grx500_core_res_l);
			gsw_dev_cell->resources = grx500_core_res_l;
		} else if (gsw_dev_cell->device_id == 1) {
			gsw_dev_cell->num_resources =
				ARRAY_SIZE(grx500_core_res_r);
			gsw_dev_cell->resources = grx500_core_res_r;
		}
	} else if (gsw_dev[devid].prod_id == PRX300) {
		gsw_dev_cell->num_resources = ARRAY_SIZE(falc_core_res);
		gsw_dev_cell->resources = falc_core_res;
	}

#endif

	return ret;
}

#ifdef CONFIG_OF
static int update_gsw_dev_cell(struct device_node *np,
			       int idx,
			       u32 devid,
			       int prod_id)
{

	int ret = 0, r = 0;
	struct gsw_cell *cell = &gsw_dev[devid].cells[idx];

	ret = of_property_read_string(np, "compatible", &cell->of_compatible);
	if (ret)
		return ret;

	ret = of_property_read_u32(np, "num_resources", &cell->num_resources);

	if (ret)
		return ret;

	strncpy(cell->name, np->name, ARRAY_SIZE(cell->name) - 1);
	cell->name[ARRAY_SIZE(cell->name) - 1] = 0;

	cell->of_node = np;

	cell->cell_id = idx;

	/* All node in this GSWSS will have same device id and prod id
	 */
	cell->device_id = devid;
	cell->prod_id = prod_id;

	/* Store the product id in gsw_dev structure
	   Individual drivers can get this info from parent driver data
	 */
	gsw_dev[devid].prod_id = prod_id;

	if (cell->num_resources) {
		cell->resources =
			kzalloc(sizeof(struct resource) * cell->num_resources,
				GFP_KERNEL);

		if (!cell->resources)
			return -ENOMEM;

		for (r = 0; r < cell->num_resources; r++) {
			if (of_address_to_resource(np, r, &cell->resources[r]))
				goto failed;
		}
	}

	if (of_node_cmp(np->name, MAC_DEV_NAME) == 0) {
		ret = gsw_add_macdev(cell, devid);

		if (ret)
			goto failed;

		gsw_dev[devid].mac_subdevs_cnt++;
	} else if ((of_node_cmp(np->name, CORE_DEV_NAME) == 0)  &&
		of_device_is_available(np)) {
		ret = gsw_add_switchdev(cell, devid);

		if (ret)
			goto failed;
		gsw_dev[devid].gsw_subdevs_cnt++;
	}

	return 0;
failed:
	if (cell->resources) {
		kfree(cell->resources);
		cell->resources = NULL;
	}
	return ret;
}

static int gsw_parse_dt(struct device_node *base_node)
{
	struct device_node *np = NULL;
	int idx = 0, ret = 0, prod_id = 0;
	u32 devid = 0;

	/* Get the Product ID */
	if (of_machine_is_compatible(GRX500_MACH_NAME))
		prod_id = GRX500;
	else if (of_machine_is_compatible(PRX300_MACH_NAME))
		prod_id = PRX300;
	else if (of_machine_is_compatible(LGM_MACH_NAME))
		prod_id = LGM;

	if (!prod_id) {
		printk("Cannot find the Product ID\n");
		return -1;
	}

	/* Get the GSWIP device ID */
	if ((!base_node) ||
	    of_property_read_u32(base_node, DEVID_STR, &devid)) {
		printk("Cannot find the Device ID\n");
		return -1;
	}

	if (devid >= ARRAY_SIZE(gsw_dev)) {
		pr_err("GSW " DEVID_STR " (%u) is out of range (0~%lu).\n",
		       devid, ARRAY_SIZE(gsw_dev));
		return -1;
	}

	memset(&gsw_dev[devid], 0, sizeof(gsw_dev[devid]));

	for_each_child_of_node(base_node, np) {
		if (of_device_is_available(np))
			gsw_dev[devid].num_devs++;
	}

	gsw_dev[devid].cells =
		kzalloc(sizeof(struct gsw_cell) * gsw_dev[devid].num_devs,
			GFP_KERNEL);

	if (!gsw_dev[devid].cells)
		return -ENOMEM;
	gsw_dev[devid].cells_need_free = 1;

	idx = 0;

	for_each_child_of_node(base_node, np) {
		ret = update_gsw_dev_cell(np, idx, devid, prod_id);

		if (ret != 0)
			goto failed;

		idx++;
	}

	return (int)devid;
failed:
	gsw_remove_data(devid);

	return -1;
}
#else

static int gsw_parse_dt(struct device_node *base_node)
{
	int idx = 0;
	u32 devid = gsw_dev_cells[0].device_id;

	memset(&gsw_dev[devid], 0, sizeof(gsw_dev));

	gsw_dev[devid].num_devs = ARRAY_SIZE(gsw_dev_cells);
	gsw_dev[devid].cells = gsw_dev_cells;

	/* Store the product id in gsw_dev structure
	 * Individual drivers can get this info from parent driver data
	 */
	gsw_dev[devid].prod_id = gsw_dev_cells[0].prod_id;

	for (idx = 0; idx < ARRAY_SIZE(gsw_dev_cells); idx++) {
		if (strcmp(gsw_dev_cells[idx].name, MAC_DEV_NAME) == 0) {
			gsw_add_macdev(&gsw_dev_cells[idx], devid);
			gsw_dev[devid].mac_subdevs_cnt++;
		} else if (strcmp(gsw_dev_cells[idx].name, CORE_DEV_NAME) == 0)
			gsw_add_switchdev(&gsw_dev_cells[idx], devid);

	}

	return (int)devid;

}
#endif

static int init_adapt_prvdata(int devid)
{
	struct adap_prv_data *adap_pdata = NULL;

	/* Allocate the Adaption Private Data */
	adap_pdata = kzalloc(sizeof(struct adap_prv_data), GFP_KERNEL);

	if (!adap_pdata) {
		printk("%s: Failed to allocate Adaption Private Data \n",
		       __func__);
		return -ENOMEM;
	}

	adap_pdata->ss_addr_base = 0;

	gsw_dev[devid].adap_ops = (void *)&adap_pdata->ops;

	/* Initialize Adaption Spin lock */
	spin_lock_init(&adap_pdata->adap_lock);

	/* Init Adaption layer function pointers */
	gswss_init_fn_ptrs(&adap_pdata->ops);

	return 0;
}

static void cfg_ptp_mux(struct platform_device *pdev, int devid)
{
	struct adap_ops *ops = gsw_dev[devid].adap_ops;
	struct device_node *np = pdev->dev.of_node;
	u32 ref, dig, bin, pps;
	u32 aux0, aux1, dummy;
	int ret;

	if (!ops->ss_get_cfg0_1588 || !ops->ss_set_cfg0_1588
	    || !ops->ss_get_cfg1_1588 || !ops->ss_set_cfg1_1588) {
		dev_err(&pdev->dev, "GSWSS does not support PTP/1588 APIs.\n");
		return;
	}

	if (!ops->ss_get_cfg1_1588(ops, &aux0, &aux1, &dummy)) {
		ret = !of_property_read_u32(np, AUX0_TRIGGER, &aux0);
		ret |= !of_property_read_u32(np, AUX1_TRIGGER, &aux1);
		if (ret)
			ops->ss_set_cfg1_1588(ops, aux1, aux0, dummy);
	}

	if (!ops->ss_get_cfg0_1588(ops, &ref, &dig, &bin, &pps)) {
		ret = !of_property_read_u32(np, REF_TIME, &ref);
		if (ret)
			dig = bin = ref;
		ret |= !of_property_read_u32(np, PPS_SEL, &pps);
		if (ret)
			ops->ss_set_cfg0_1588(ops, ref, dig, bin, pps);
	}
}

static int gsw_remove_devices(struct device *parent)
{
	struct gswss *gswdev = dev_get_drvdata(parent);
	int i = 0;

	cdev_del(&gswdev->gswss_cdev);
	gsw_remove_data(parent->id);

	if (gswdev->adap_ops)
		kfree(gswdev->adap_ops);

	for (i = 0; i < gswdev->gsw_subdevs_cnt; i++) {
		if (gswdev->core_dev[i])
			platform_device_put(gswdev->core_dev[i]);
	}

	for (i = 0; i < gswdev->mac_subdevs_cnt; i++)
		platform_device_put(gswdev->mac_dev[i]);

	return 0;
}

static int gsw_add_device(struct device *parent, int id,
			  struct gsw_cell *cell,
			  struct resource *mem_base,
			  int irq_base)
{
	struct resource *res;
	struct platform_device *pdev;
	int ret = -ENOMEM;
	int platform_id;
	int r;

	if (id == PLATFORM_DEVID_AUTO)
		platform_id = id;
	else
		platform_id = id + cell->cell_id;

	pdev = platform_device_alloc(cell->name, platform_id);

	if (!pdev)
		goto fail_alloc;

	res = kzalloc(sizeof(*res) * cell->num_resources, GFP_KERNEL);

	if (!res)
		goto fail_device;

	pdev->dev.parent = parent;

	pdev->dev.of_node = cell->of_node;

	if (cell->pdata_size) {
		ret = platform_device_add_data(pdev,
					       cell->platform_data,
					       cell->pdata_size);

		if (ret)
			goto fail_res;
	}

	if (cell->drv_data_size) {
		platform_set_drvdata(pdev, cell->drv_data);
	}

	for (r = 0; r < cell->num_resources; r++) {
		res[r].name = cell->resources[r].name;
		res[r].flags = cell->resources[r].flags;

		/* Find out base to use */
		if ((cell->resources[r].flags & IORESOURCE_MEM) && mem_base) {
			res[r].parent = mem_base;
			res[r].start = mem_base->start +
				       cell->resources[r].start;
			res[r].end = mem_base->start +
				     cell->resources[r].end;
		} else if (cell->resources[r].flags & IORESOURCE_IRQ) {
			res[r].start = irq_base +
				       cell->resources[r].start;
			res[r].end   = irq_base +
				       cell->resources[r].end;
		} else {
			res[r].parent = cell->resources[r].parent;
			res[r].start = cell->resources[r].start;
			res[r].end   = cell->resources[r].end;
		}
	}

	ret = platform_device_add_resources(pdev, res, cell->num_resources);

	if (ret)
		goto fail_res;

	ret = platform_device_add(pdev);

	if (ret)
		goto fail_res;

	kfree(res);

	return 0;

fail_res:
	kfree(res);
fail_device:
	platform_device_put(pdev);
fail_alloc:
	return ret;
}

static int gsw_add_devices(struct device *parent, int id,
			   struct gsw_cell *cells, int n_devs,
			   struct resource *mem_base,
			   int irq_base)
{
	int i;
	int ret;

	for (i = 0; i < n_devs; i++) {
		ret = gsw_add_device(parent, id, &cells[i], mem_base,
				     irq_base);

		if (ret)
			goto fail;
	}

	return 0;

fail:

	if (i)
		gsw_remove_devices(parent);

	return ret;
}

static u64 _rxfifo_err_count(struct mac_ops *ops)
{
	u32 val_lo = 0, val_hi = 0;
	u64 val = 0;

	if (!ops)
		return 0;
	ops->xgmac_reg_rd(ops, MMC_RXFIFOOVERFLOW_LO, &val_lo);
	ops->xgmac_reg_rd(ops, MMC_RXFIFOOVERFLOW_LO + 4, &val_hi);
	val = ((u64)val_hi << 32) | val_lo;

	return val;
}

/* This API will get executed by workqueue
 * if per MAC rxfifo_err exceeds rxfifo_err_threshold,
 * it resets corresponding MAC adaption layer
 */
static int rxfifo_err_poll(struct delayed_work *work)
{
	struct mac_rxfifo_poll *rxfifo_err =
		container_of(work, struct mac_rxfifo_poll, work);
	u32 devid = 0;
	int mac_cnt = gsw_get_mac_subifcnt(devid);
	int i = 0;
	struct mac_ops *ops;
	struct mac_prv_data *pdata;
	u64 val = 0;
	int err_cnt = 0;

	for (i = 0; i < mac_cnt; i++) {
		ops = gsw_get_mac_ops(devid, MAC_2 + i);
		if (!ops)
			continue;

		pdata = GET_MAC_PDATA(ops);
		if (!pdata || !pdata->rxfifo_err.rxfifo_err_threshold)
			continue;

		val = _rxfifo_err_count(ops);

		if (!pdata->rxfifo_err.rxfifo_err_cnt) {
			pdata->rxfifo_err.rxfifo_err_cnt = val;
			err_cnt = val;
		} else {
			err_cnt = val - pdata->rxfifo_err.rxfifo_err_cnt;
		}

		if (err_cnt > pdata->rxfifo_err.rxfifo_err_threshold) {
			int linksts = ops->get_link_sts(ops);
			int stat_linksts = gswss_get_stat_linkstatus(ops);
			DECLARE_WAIT_QUEUE_HEAD(link_wait_queue);
			u64 new_cnt = 0;

			/* Disable MAC receiver and force link down */
			ops->mac_receiver_enable(ops, 0);
			ops->set_link_sts(ops, GSW_PORT_LINK_DOWN);
			wait_event_interruptible_timeout(link_wait_queue,
							 gswss_get_stat_linkstatus(ops) == 0,
							 msecs_to_jiffies(10));

			/* Reset MAC Adaption Layer */
			gswss_adap_reset(ops, 1);
			pdata->rxfifo_err.rxfifo_err_cnt = val;
			mdelay(1);
			gswss_adap_reset(ops, 0);

			/* Restore link status and enable MAC receiver */
			mdelay(10);
			ops->set_link_sts(ops, linksts);
			ops->mac_receiver_enable(ops, 1);
			wait_event_interruptible_timeout(link_wait_queue,
							 gswss_get_stat_linkstatus(ops) == stat_linksts,
							 msecs_to_jiffies(10));

			/* Wait until the counter returned by _rxfifo_err_count
			 * stops changing or timeout
			 */
			wait_event_interruptible_timeout(link_wait_queue,
							 ({
								new_cnt = _rxfifo_err_count(ops);
								new_cnt == val || (val = new_cnt, 0);
							 }),
							 msecs_to_jiffies(20));
			pdata->rxfifo_err.rxfifo_err_cnt = val;
		}
	}

	/* reschedule to check later after 1 sec*/
	schedule_delayed_work(&rxfifo_err->work, msecs_to_jiffies(1000));

	return 1;
}

static void cfg_rxfifo_err_poll(struct platform_device *pdev, int devid)
{
	int mac_cnt = gsw_get_mac_subifcnt(devid);
	struct mac_ops *ops;
	struct mac_prv_data *pdata;
	int idx;

	for (idx = 0; idx < mac_cnt; idx++) {
		ops = gsw_get_mac_ops(devid, MAC_2 + idx);
		pdata = GET_MAC_PDATA(ops);

		if (pdata->rxfifo_err.rxfifo_err_threshold) {
			/* Init and schedule RxFifo Error Poll
			 * for all MAC's in every 1 sec
			 */
			dev_dbg(&pdev->dev, "%s: start rxfifo_err_poll work queue\n", __func__);
			INIT_DELAYED_WORK(&pdata->rxfifo_err.work, (work_func_t)rxfifo_err_poll);
			schedule_delayed_work(&pdata->rxfifo_err.work, msecs_to_jiffies(1000));
			break;
		}
	}
}

/* Counters update interval = 3[min] -> 180'000[ms] */
#define CNT_UPDATE_TIME 180000U

static void ctp_cnt_update_work(struct work_struct *work)
{
	gsw_ctp_cnt_info_t *ctp_cnt_info =
		container_of(work, gsw_ctp_cnt_info_t, cnt_update_work.work);
	struct core_ops *ops = gsw_get_swcore_ops(ctp_cnt_info->devid);
	GSW_RMON_CTP_cnt_t rmon_ctp_cnt;
	u16 i;

	for (i = 0; i < ctp_cnt_info->max_ctp; i++) {
		rmon_ctp_cnt.nCTP = i;
		ops->gsw_rmon_ops.RMON_CTP_Get(ops, &rmon_ctp_cnt);
	}
	schedule_delayed_work(&ctp_cnt_info->cnt_update_work,
			      msecs_to_jiffies(CNT_UPDATE_TIME));
}

static void gsw_create_ctp_cnt_update_work(struct device *dev, u32 devid)
{
	struct core_ops *ops = gsw_get_swcore_ops(devid);
	ethsw_api_dev_t *gswdev = container_of(ops, ethsw_api_dev_t, ops);

	gswdev->ctp_cnt_info.devid = devid;
	gswdev->ctp_cnt_info.max_ctp = gswdev->num_of_ctp;
	gswdev->ctp_cnt_info.cnt = devm_kzalloc(dev,
				sizeof(gsw_ctp_cnt_t) * gswdev->ctp_cnt_info.max_ctp,
				GFP_ATOMIC);
	if (gswdev->ctp_cnt_info.cnt == NULL)
		return;
	INIT_DELAYED_WORK(&gswdev->ctp_cnt_info.cnt_update_work,
			  ctp_cnt_update_work);
	schedule_delayed_work(&gswdev->ctp_cnt_info.cnt_update_work,
			      msecs_to_jiffies(1));
}

static int lmac_strobe_cfg(struct device *dev, struct gswss *gswss)
{
	int nco1, nco2;
	u32 nco3, nco4;
	u32 strobe;
	u64 val;
	u32 freq;

	if (!gswss->adap_ops->ss_get_nco || !gswss->adap_ops->ss_set_nco)
		return -ENOTSUPP;

	if (gswss->prod_id == PRX300)
		strobe = PRX300_STROBE;
	else if (gswss->prod_id == LGM)
		strobe = LGM_STROBE;
	else
		return -ENOTSUPP;

	val = clk_get_rate(gswss->clk);
	nco1 = gswss->adap_ops->ss_get_nco(gswss->adap_ops, 1);
	val *= nco1;
	val >>= 24;
	freq = (u32)val;
	val = strobe;
	val <<= 24;
	nco3 = (u32)div64_u64(val, freq);
	dev_dbg(dev, "GSWSS NCO3 0x%06x\n", nco3);
	gswss->adap_ops->ss_set_nco(gswss->adap_ops, nco3, 3);

	val = clk_get_rate(gswss->clk);
	nco2 = gswss->adap_ops->ss_get_nco(gswss->adap_ops, 2);
	val *= nco2;
	val >>= 24;
	freq = (u32)val;
	val = strobe;
	val <<= 24;
	nco4 = (u32)div64_u64(val, freq);
	dev_dbg(dev, "GSWSS NCO3 0x%06x\n", nco4);
	gswss->adap_ops->ss_set_nco(gswss->adap_ops, nco4, 4);

	return 0;
}

static int gsw_dev_probe(struct platform_device *pdev)
{
	u32 devid = 0;
	int ret = 0;
	struct device *dev = &pdev->dev;
	struct device_node *base_node = dev->of_node;
	u32 prod_id = 0;
	struct clk *clk;

	/* parse Device tree */
	ret = gsw_parse_dt(base_node);

	if (ret < 0) {
		pr_crit("%s: Failed to parse device tree \n", __func__);
		return -1;
	}

	devid = (u32)ret;
	prod_id = gsw_dev[devid].prod_id;

	/* Store the device id in parent device ID */
	dev->id = devid;

	clk = devm_clk_get(dev, "freq");
	if (!IS_ERR(clk)) {
		gsw_dev[devid].clk = clk;
		clk_prepare_enable(clk);
	} else
		pr_err("+++ GSW No Freq clock\n");

	clk = devm_clk_get(dev, "gate");
	if (!IS_ERR(clk)) {
		gsw_dev[devid].gclk = clk;
		clk_prepare_enable(clk);
	} else
		pr_err("+++ GSW No Gate clock\n");

	if (prod_id == PRX300 || prod_id == LGM) {
		struct reset_control *gswss_rst = gsw_reset_get(pdev);
		if (!gswss_rst) {
			dev_err(&pdev->dev, "failed to get gswip reset:\n");
			goto ERROR;
		}

		/* Make Sure GSWIP is out of reset in RCU reg */
		gsw_rst(gswss_rst);

		/* Init Adaption private data */
		ret = init_adapt_prvdata(devid);

		if (ret) {
			pr_crit("%s: Failed to allocate Adaption Priv Data\n",
				__func__);
			goto ERROR;
		}
	}

	/* Set gsw_dev[devid] inside parent Driver data */
	dev_set_drvdata(dev, &gsw_dev[devid]);

	/* Add devices */
	ret = gsw_add_devices(dev, PLATFORM_DEVID_AUTO, gsw_dev[devid].cells,
			      gsw_dev[devid].num_devs,
			      NULL, 0);

	if (!ret && gsw_dev[devid].clk)
		lmac_strobe_cfg(dev, &gsw_dev[devid]);

	/* Config PTP/1588 Mux */
	if (ret == 0 && (prod_id == PRX300 || prod_id == LGM))
		cfg_ptp_mux(pdev, devid);

	if (ret == 0 && prod_id != GRX500)
		cfg_rxfifo_err_poll(pdev, devid);

	if (ret == 0 && prod_id == LGM)
		gsw_create_ctp_cnt_update_work(dev, devid);

	return ret;

ERROR:
	if (gsw_dev[devid].gclk) {
		clk_disable_unprepare(gsw_dev[devid].gclk);
		gsw_dev[devid].gclk = NULL;
	}
	if (gsw_dev[devid].clk) {
		clk_disable_unprepare(gsw_dev[devid].clk);
		gsw_dev[devid].clk = NULL;
	}
	return -1;
}

struct adap_ops *gsw_get_adap_ops(u32 devid)
{
	if (gsw_dev[devid].adap_ops)
		return ((struct adap_ops *)(gsw_dev[devid].adap_ops));

	return NULL;
}
EXPORT_SYMBOL(gsw_get_adap_ops);

/* Upper layer gives MAC Idx as 2, 3, 4, ... */
struct mac_ops *gsw_get_mac_ops(u32 devid, u32 mac_idx)
{
	u32 macid = mac_idx - MAC_2;

	if (macid >= gsw_dev[devid].mac_subdevs_cnt)
		return NULL;

	if (gsw_dev[devid].mac_dev[macid])
		return (platform_get_drvdata(gsw_dev[devid].mac_dev[macid]));

	return NULL;
}
EXPORT_SYMBOL(gsw_get_mac_ops);

/* API to get GSWIP Core Layer operations */
struct core_ops *gsw_get_swcore_ops(u32 devid)
{
	if (gsw_dev[devid].core_dev[devid])
		return (platform_get_drvdata(gsw_dev[devid].core_dev[devid]));

	return NULL;
}
EXPORT_SYMBOL(gsw_get_swcore_ops);

struct platform_device *gsw_get_coredev(u32 devid)
{
	if (gsw_dev[devid].core_dev[devid])
		return (gsw_dev[devid].core_dev[devid]);

	return NULL;
}
EXPORT_SYMBOL(gsw_get_coredev);

struct platform_device *gsw_get_macdev(u32 devid, u32 mac_idx)
{
	u32 macid = mac_idx - MAC_2;

	if (macid > gsw_dev[devid].mac_subdevs_cnt) {
		pr_crit("%s: No. of Mac Idx given is more than Mac Sub devs"
			"supported\n",
			__func__);
		return NULL;
	}

	if (gsw_dev[devid].mac_dev[macid])
		return (gsw_dev[devid].mac_dev[macid]);

	return NULL;
}
EXPORT_SYMBOL(gsw_get_macdev);

u32 gsw_get_mac_subifcnt(u32 devid)
{
	return (gsw_dev[devid].mac_subdevs_cnt);
}
EXPORT_SYMBOL(gsw_get_mac_subifcnt);

u32 gsw_get_total_devs(u32 devid)
{
	return (gsw_dev[devid].num_devs);
}
EXPORT_SYMBOL(gsw_get_total_devs);

static const struct of_device_id gsw_dev_match[] = {
	{ .compatible = "intel,gswdev" },
	{},
};
MODULE_DEVICE_TABLE(of, gsw_dev_match);

static struct platform_driver gsw_dev_driver = {
	.probe = gsw_dev_probe,
	.driver = {
		.name = "gswdev",
		.owner = THIS_MODULE,
		.of_match_table = gsw_dev_match,
	},
};

module_platform_driver(gsw_dev_driver);

MODULE_AUTHOR("Intel");
MODULE_DESCRIPTION("Intel GSWIP Device driver for Adding devices");
MODULE_LICENSE("GPL");

