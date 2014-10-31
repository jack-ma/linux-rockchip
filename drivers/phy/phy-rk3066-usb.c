/*
 * Rockchip RK3188 and RK3066 USB phy driver
 *
 * Copyright (C) 2014 Romain Perier <romain.perier@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/clk.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/reset.h>


#define UOC_CON0			0x0
#define UOC_CON0_TXBITSTUFF_H		BIT(15)
#define UOC_CON0_TXBITSTUFF_L		BIT(14)
#define UOC_CON0_SIDDQ			BIT(13)
#define UOC_CON0_PORT_RESET		BIT(12)
#define UOC_CON0_REFCLK_MASK		(0x3 << 10)
#define UOC_CON0_REFCLK_CORE		(0x2 << 10)
#define UOC_CON0_REFCLK_XO		(0x1 << 10)
#define UOC_CON0_REFCLK_CRYSTAL		0
#define UOC_CON0_BYPASS			BIT(9) /* rk3188 and only phy0 */
#define UOC_CON0_BYPASS_DM		BIT(8) /* rk3188 and only phy0 */
#define UOC_CON0_REFCLK_DIV_MASK	(0x3 << 8) /* rk3066a */

#define UOC_CON0_OTG_TUNE_MASK		(0x7 << 5)
#define UOC_CON0_OTG_DISABLE		BIT(4)
#define UOC_CON0_COMPDIS_TUNE_MASK	(0x7 << 1)
#define UOC_CON0_SUSPEND_PD		BIT(0)

#define UOC_CON1			0x4
#define UOC_CON1_TXRISE_TUNE_MASK	(0x3 << 14)
#define UOC_CON1_TXHSXV_TUNE_MASK	(0x3 << 12)
#define UOC_CON1_TXVREF_TUNE_MASK	(0xf << 8)
#define UOC_CON1_TXFSLS_TUNE_MASK	(0xf << 4)
#define UOC_CON1_TXPREEMP_TUNE		BIT(3)
#define UOC_CON1_SQRXTUNE		(0x7 << 0)

#define UOC_CON2			0x8
#define UOC_CON2_ADP_PROBLE		BIT(15) /* rk3188 */
#define UOC_CON2_ADP_DISCHARGE		BIT(14) /* rk3188 */
#define UOC_CON2_ADP_CHARGE		BIT(13) /* rk3188 */
#define UOC_CON2_TXRES_TUNE_MASK	(0x3 << 11)  /* rk3188 */
#define UOC_CON2_SCALEDOWN_MASK		(0x3 << 11) /* rk3066a uoc0 */

#define UOC_CON2_SLEEP_MODE		BIT(10)

#define UOC_CON2_VREGTUNE		BIT(9) /* rk3066 */
#define UOC_CON2_UTMI_TERMSELECT	BIT(8) /* rk3066 */
#define UOC_CON2_UTMI_XCVRSELECT_MASK	(0x3 << 6) /* rk3066 */
#define UOC_CON2_UTMI_OPMODE_MASK	(0x3 << 4) /* rk3066 */
#define UOC_CON2_UTMI_SUSPEND_DISABLE	BIT(3) /* rk3066 */
#define UOC_CON2_RETENTION		BIT(8) /* rk3188 */
#define UOC_CON2_REFCLK_FREQ_MASK	(0x7 << 5) /* rk3188 */
#define UOC_CON2_TX_PREEMP_TUNE_MASK	(0x3 << 3) /* rk3188 */

#define UOC_CON2_SOFT_CTRL		BIT(2)
#define UOC_CON2_VBUS_VALID_EXTSEL	BIT(1)
#define UOC_CON2_VBUS_VALID_EXT		BIT(0)

/* only rk3188 and rk3066 uoc1 */
#define UOC_CON3			0xc
#define UOC_CON3_BVALID_INT_PEND	BIT(15) /* only phy0 */
#define UOC_CON3_BVALID_INT_ENABLE	BIT(14) /* only phy0 */
#define UOC_CON3_UTMI_TERMSELECT	BIT(5)
#define UOC_CON3_UTMI_XCVRSELECT_MASK	(0x3 << 3)
#define UOC_CON3_UTMI_OPMODE_MASK	(0x3 << 1)
#define UOC_CON3_UTMI_SUSPEND_DISABLE	BIT(0)

#define UOC_CON3_SCALEDOWN_MASK		(0x3 << 6) /* rk3066a uoc1 */



////////////////////////////

#define REG_ISCR			0x00
#define REG_PHYCTL			0x04
#define REG_PHYBIST			0x08
#define REG_PHYTUNE			0x0c

#define PHYCTL_DATA			BIT(7)

#define SUNXI_AHB_ICHR8_EN		BIT(10)
#define SUNXI_AHB_INCR4_BURST_EN	BIT(9)
#define SUNXI_AHB_INCRX_ALIGN_EN	BIT(8)
#define SUNXI_ULPI_BYPASS_EN		BIT(0)

/* Common Control Bits for Both PHYs */
#define PHY_PLL_BW			0x03
#define PHY_RES45_CAL_EN		0x0c

/* Private Control Bits for Each PHY */
#define PHY_TX_AMPLITUDE_TUNE		0x20
#define PHY_TX_SLEWRATE_TUNE		0x22
#define PHY_VBUSVALID_TH_SEL		0x25
#define PHY_PULLUP_RES_SEL		0x27
#define PHY_OTG_FUNC_EN			0x28
#define PHY_VBUS_DET_EN			0x29
#define PHY_DISCON_TH_SEL		0x2a

#define MAX_PHYS			3

struct rk3066_usb_phy_data
{
	struct regulator *vbus;
};

static int rk3066_usb_phy_init(struct phy *phy)
{
	return 0;
}

static int rk3066_usb_phy_exit(struct phy *phy)
{
	return 0;
}

static int rk3066_usb_phy_power_on(struct phy *phy)
{
	return 0;
}

static int rk3066_usb_phy_power_off(struct phy *phy)
{
	return 0;
}

static struct phy_ops rk3066_usb_phy_ops = {
	.init		= rk3066_usb_phy_init,
	.exit		= rk3066_usb_phy_exit,
	.power_on	= rk3066_usb_phy_power_on,
	.power_off	= rk3066_usb_phy_power_off,
	.owner		= THIS_MODULE,
};

static int rk3066_usb_phy_probe(struct platform_device *pdev)
{
	struct rk3066_usb_phy_data *data;
	struct device *dev = &pdev->dev;

	if (!pdev->dev.of_node)
		return -ENODEV;

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;
	dev_set_drvdata(dev, data);
	return 0;
}

static const struct of_device_id rk3066_usb_phy_of_match[] = {
	{ .compatible = "rockchip,rk3066-usb-phy" },
	{ .compatible = "rockchip,rk3188-usb-phy" },
	{ },
};
MODULE_DEVICE_TABLE(of, rk3066_usb_phy_of_match);

static struct platform_driver rk3066_usb_phy_driver = {
	.probe	= rk3066_usb_phy_probe,
	.driver = {
		.of_match_table	= rk3066_usb_phy_of_match,
		.name  = "rk3066-usb-phy",
		.owner = THIS_MODULE,
	}
};
module_platform_driver(rk3066_usb_phy_driver);

MODULE_DESCRIPTION("Rockchip RK3188 and RK3066 USB phy driver");
MODULE_AUTHOR("Romain Perier <romain.perier@gmail.com>");
MODULE_LICENSE("GPL v2");
