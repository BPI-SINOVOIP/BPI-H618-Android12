/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP21  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/pmu_axp1530.h>
#include <sunxi_power/axp.h>
#include <asm/arch/pmic_bus.h>
#include <sys_config.h>
#include <sunxi_power/pmu_ext.h>
/*#include <power/sunxi/pmu.h>*/

#ifdef PMU_DEBUG
#define axp_info(fmt...) tick_printf("[axp][info]: " fmt)
#define axp_err(fmt...) tick_printf("[axp][err]: " fmt)
#else
#define axp_info(fmt...)
#define axp_err(fmt...) tick_printf("[axp][err]: " fmt)
#endif

int axp1530_exist;
static int pmu_axp1530_necessary_reg_enable(void)
{
	__attribute__((unused)) u8 reg_value;
#ifdef CONFIG_AXP1530A_NECESSARY_REG_ENABLE
	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_WRITE_LOCK, &reg_value))
		return -1;
	reg_value |= 0x5;
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_WRITE_LOCK, reg_value))
		return -1;

	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_ERROR_MANAGEMENT, &reg_value))
		return -1;
	reg_value |= 0x8;
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_ERROR_MANAGEMENT, reg_value))
		return -1;

	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_DCDC_DVM_PWM_CTL, &reg_value))
		return -1;
	reg_value |= (0x1 << 5);
	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_DCDC_DVM_PWM_CTL, reg_value))
		return -1;
#endif
	return 0;
}

static int pmu_axp1530_probe(void)
{
	u8 pmu_chip_id;
	int axp_bus_num;

	script_parser_fetch(FDT_PATH_POWER_SPLY, "axp_bus_num", &axp_bus_num, AXP1530_DEVICE_ADDR);

	if (pmic_bus_init(axp_bus_num, AXP1530_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}

	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_VERSION, &pmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}
	pmu_chip_id &= 0XCF;
	axp1530_exist = true;

	if (pmu_chip_id == AXP1530_CHIP_ID || pmu_chip_id == AXP313A_CHIP_ID || pmu_chip_id == AXP313B_CHIP_ID
			|| pmu_chip_id == AXP323_CHIP_ID) {
		/*pmu type AXP1530*/
		pmu_axp1530_necessary_reg_enable();
		tick_printf("PMU: AXP1530\n");
		return 0;
	}

	return -1;
}

static int pmu_axp1530_set_dcdc_mode(const char *name, int mode)
{
	u8 reg_value = 0, mask = 0;

	if (!strncmp(name, "dcdc1_mode", sizeof("dcdc1_mode")))
		mask = AXP1530_DCDC1_PWM_BIT;

	if (!strncmp(name, "dcdc2_mode", sizeof("dcdc2_mode")))
		mask = AXP1530_DCDC2_PWM_BIT;

	if (!strncmp(name, "dcdc3_mode", sizeof("dcdc3_mode")))
		mask = AXP1530_DCDC3_PWM_BIT;

	if (pmic_bus_read(AXP1530_RUNTIME_ADDR, AXP1530_DCDC_MODESET, &reg_value))
		return -1;

	reg_value &= ~(1 << mask);
	reg_value |= (mode << mask);

	if (pmic_bus_write(AXP1530_RUNTIME_ADDR, AXP1530_DCDC_MODESET, reg_value))
		return -1;

	return 0;
}

bool pmu_axp1530_get_exist(void)
{
	return axp1530_exist;
}


U_BOOT_PMU_EXT_INIT(pmu_axp1530) = {
	.pmu_ext_name     = "pmu_axp1530",
	.probe             = pmu_axp1530_probe,
	.set_dcdc_mode     = pmu_axp1530_set_dcdc_mode,
	.get_exist	   = pmu_axp1530_get_exist,
};

