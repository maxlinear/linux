// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 Intel Corporation.
 *
 * Limitations:
 * - The hardware supports fixed period & configures only 2-wire mode.
 * - Supports normal polarity. Does not support changing polarity.
 * - When PWM is disabled, output of PWM will become 0(inactive). It doesn't
 *   keep track of running period.
 * - When duty cycle is changed, PWM output may be a mix of previous setting
 *   and new setting for the first period. From second period, the output is
 *   based on new setting.
 * - It is a dedicated PWM fan controller. There are no other consumers for
 *   this PWM controller.
 */
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/pwm.h>
#include <linux/regmap.h>
#include <linux/reset.h>

#define LGM_PWM_FAN_CON0		0x0
#define LGM_PWM_FAN_EN_EN		BIT(0)
#define LGM_PWM_FAN_EN_DIS		0x0
#define LGM_PWM_FAN_EN_MSK		BIT(0)
#define LGM_PWM_FAN_MODE_2WIRE		0x0
#define LGM_PWM_FAN_MODE_MSK		BIT(1)
#define LGM_PWM_FAN_DC_MSK		GENMASK(23, 16)

#define LGM_PWM_FAN_CON1		0x4
#define LGM_PWM_FAN_MAX_RPM_MSK		GENMASK(15, 0)

#define LGM_PWM_MAX_RPM			(BIT(16) - 1)
#define LGM_PWM_DEFAULT_RPM		4000
#define LGM_PWM_MAX_DUTY_CYCLE		(BIT(8) - 1)

#define LGM_PWM_DC_BITS			8

#define LGM_PWM_PERIOD_2WIRE_NS		(40 * NSEC_PER_MSEC)

struct lgm_pwm_chip {
	struct pwm_chip chip;
	struct regmap *regmap;
	u32 period;
};

static inline struct lgm_pwm_chip *to_lgm_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct lgm_pwm_chip, chip);
}

static int pwm_update_dc(struct lgm_pwm_chip *pc, u32 val)
{
	return regmap_update_bits(pc->regmap, LGM_PWM_FAN_CON0, LGM_PWM_FAN_DC_MSK,
				  FIELD_PREP(LGM_PWM_FAN_DC_MSK, val));
}

static int lgm_pwm_apply(struct pwm_chip *chip, struct pwm_device *pwm,
			 const struct pwm_state *state)
{
	struct lgm_pwm_chip *pc = to_lgm_pwm_chip(chip);
	u32 val;

	val = DIV_ROUND_CLOSEST(state->duty_cycle * LGM_PWM_MAX_DUTY_CYCLE,
				state->period);
	val = min_t(u32, val, LGM_PWM_MAX_DUTY_CYCLE);

	return pwm_update_dc(pc, val);
}

static const struct pwm_ops lgm_pwm_ops = {
	.apply		= lgm_pwm_apply,
	.owner		= THIS_MODULE,
};

static void lgm_pwm_init(struct lgm_pwm_chip *pc)
{
	struct regmap *regmap = pc->regmap;
	u32 con0_val;

	con0_val = FIELD_PREP(LGM_PWM_FAN_MODE_MSK, LGM_PWM_FAN_MODE_2WIRE);
	pc->period = LGM_PWM_PERIOD_2WIRE_NS;
	regmap_update_bits(regmap, LGM_PWM_FAN_CON1, LGM_PWM_FAN_MAX_RPM_MSK,
			   LGM_PWM_DEFAULT_RPM);
	regmap_update_bits(regmap, LGM_PWM_FAN_CON0, LGM_PWM_FAN_MODE_MSK,
			   con0_val);
	regmap_update_bits(regmap, LGM_PWM_FAN_CON0,
			   LGM_PWM_FAN_EN_MSK, LGM_PWM_FAN_EN_EN);
}

static const struct regmap_config lgm_pwm_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
};

static void lgm_clk_release(void *data)
{
	struct clk *clk = data;

	clk_disable_unprepare(clk);
}

static int lgm_clk_enable(struct device *dev, struct clk *clk)
{
	int ret;

	ret = clk_prepare_enable(clk);
	if (ret)
		return ret;

	return devm_add_action_or_reset(dev, lgm_clk_release, clk);
}

static void lgm_reset_control_release(void *data)
{
	struct reset_control *rst = data;

	reset_control_assert(rst);
}

static int lgm_reset_control_deassert(struct device *dev, struct reset_control *rst)
{
	int ret;

	ret = reset_control_deassert(rst);
	if (ret)
		return ret;

	return devm_add_action_or_reset(dev, lgm_reset_control_release, rst);
}

static int lgm_pwm_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct reset_control *rst;
	struct lgm_pwm_chip *pc;
	void __iomem *io_base;
	struct clk *clk;
	int ret;

	pc = devm_kzalloc(dev, sizeof(*pc), GFP_KERNEL);
	if (!pc)
		return -ENOMEM;

	io_base = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(io_base))
		return PTR_ERR(io_base);

	pc->regmap = devm_regmap_init_mmio(dev, io_base, &lgm_pwm_regmap_config);
	if (IS_ERR(pc->regmap))
		return dev_err_probe(dev, PTR_ERR(pc->regmap),
				     "failed to init register map\n");

	clk = devm_clk_get(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk), "failed to get clock\n");

	ret = lgm_clk_enable(dev, clk);
	if (ret)
		return dev_err_probe(dev, ret, "failed to enable clock\n");

	rst = devm_reset_control_get_exclusive(dev, NULL);
	if (IS_ERR(rst))
		return dev_err_probe(dev, PTR_ERR(rst),
				     "failed to get reset control\n");

	ret = lgm_reset_control_deassert(dev, rst);
	if (ret)
		return dev_err_probe(dev, ret, "cannot deassert reset control\n");

	pc->chip.dev = dev;
	pc->chip.ops = &lgm_pwm_ops;
	pc->chip.npwm = 1;

	lgm_pwm_init(pc);

	ret = devm_pwmchip_add(dev, &pc->chip);
	if (ret < 0)
		return dev_err_probe(dev, ret, "failed to add PWM chip\n");

	return 0;
}

static const struct of_device_id lgm_pwm_of_match[] = {
	{ .compatible = "intel,lgm-pwm" },
	{ }
};
MODULE_DEVICE_TABLE(of, lgm_pwm_of_match);

static struct platform_driver lgm_pwm_driver = {
	.driver = {
		.name = "intel-pwm",
		.of_match_table = lgm_pwm_of_match,
	},
	.probe = lgm_pwm_probe,
};
module_platform_driver(lgm_pwm_driver);

MODULE_LICENSE("GPL v2");
