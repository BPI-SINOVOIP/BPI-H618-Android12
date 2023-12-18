/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI SY8827G  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/pmu_sy8827g.h>
#include <sunxi_power/pmu_ext.h>
#include <asm/arch/pmic_bus.h>

/*#include <power/sunxi/pmu.h>*/

#ifdef PMU_DEBUG
#define axp_info(fmt...) tick_printf("[pmu_ext][info]: " fmt)
#define axp_err(fmt...) tick_printf("[pmu_ext][err]: " fmt)
#else
#define axp_info(fmt...)
#define axp_err(fmt...) tick_printf("[pmu_ext][err]: " fmt)
#endif

static bool sy8827g_exist;

int pmu_sy8827g_probe(void)
{
	tick_printf("EXT: EXT_probe\n");
	u8 pmu_chip_id;
	if (pmic_bus_init(SY8827G_DEVICE_ADDR, SY8827G_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}

	if (pmic_bus_read(SY8827G_RUNTIME_ADDR, SY8827G_ID1, &pmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}

	pmu_chip_id &= 0xE0;

	if (pmu_chip_id == 0x80) {
		sy8827g_exist = true;
		tick_printf("EXT: SY8827G\n");
		return 0;
	}

	tick_printf("EXT: NO FOUND\n");

	return -1;
}

int pmu_sy8827g_set_dcdc_mode(const char *name, int mode)
{
	u8 reg_value = 0, reg = 0;

	if (!strncmp(name, "ext_dcdc0_mode", sizeof("ext_dcdc0_mode")))
		reg = SY8827G_VSEL0;

	if (!strncmp(name, "ext_dcdc1_mode", sizeof("ext_dcdc1_mode")))
		reg = SY8827G_VSEL1;

	if (pmic_bus_read(SY8827G_RUNTIME_ADDR, reg, &reg_value))
		return -1;

	reg_value &= ~(1 << SY8827G_DCDC_PWM_BIT);
	reg_value |= (mode << SY8827G_DCDC_PWM_BIT);

	if (pmic_bus_write(SY8827G_RUNTIME_ADDR, reg, reg_value))
		return -1;

	pmic_bus_read(SY8827G_RUNTIME_ADDR, reg, &reg_value);
	tick_printf("SY8827G:reg%x:%x\n", reg, reg_value);
	return 0;
}

inline bool get_sy8827g(void)
{
	return sy8827g_exist;
}

U_BOOT_PMU_EXT_INIT(pmu_sy8827g) = {
	.pmu_ext_name	  = "pmu_sy8827g",
	.probe			  = pmu_sy8827g_probe,
	.set_dcdc_mode     = pmu_sy8827g_set_dcdc_mode,
	.get_exist	   = get_sy8827g,
};
