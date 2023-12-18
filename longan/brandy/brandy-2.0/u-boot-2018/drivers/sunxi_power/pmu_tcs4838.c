/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI TCS4838  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#include <common.h>
#include <sunxi_power/pmu_tcs4838.h>
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

static bool tcs4838_exist;

int pmu_tcs4838_probe(void)
{
	tick_printf("TCS: TCS_probe\n");
	u8 pmu_chip_id;
	if (pmic_bus_init(TCS4838_DEVICE_ADDR, TCS4838_RUNTIME_ADDR)) {
		tick_printf("%s pmic_bus_init fail\n", __func__);
		return -1;
	}

	if (pmic_bus_read(TCS4838_RUNTIME_ADDR, TCS4838_ID1, &pmu_chip_id)) {
		tick_printf("%s pmic_bus_read fail\n", __func__);
		return -1;
	}

	pmu_chip_id &= 0xE0;

	if (pmu_chip_id == 0x80) {
		tcs4838_exist = true;
		tick_printf("TCS: TCS4838\n");
		return 0;
	}

	tick_printf("TCS: NO FOUND\n");

	return -1;
}

int pmu_tcs4838_set_dcdc_mode(const char *name, int mode)
{
	u8 reg_value = 0, reg = 0;

	if (!strncmp(name, "ext_dcdc0_mode", sizeof("tcs_dcdc0_mode")))
		reg = TCS4838_VSEL0;

	if (!strncmp(name, "ext_dcdc1_mode", sizeof("tcs_dcdc1_mode")))
		reg = TCS4838_VSEL1;

	if (pmic_bus_read(TCS4838_RUNTIME_ADDR, reg, &reg_value))
		return -1;

	reg_value &= ~(1 << TCS4838_DCDC_PWM_BIT);
	reg_value |= (mode << TCS4838_DCDC_PWM_BIT);

	if (pmic_bus_write(TCS4838_RUNTIME_ADDR, reg, reg_value))
		return -1;

	pmic_bus_read(TCS4838_RUNTIME_ADDR, reg, &reg_value);
	tick_printf("PTCS4838:reg%x:%x\n", reg, reg_value);
	return 0;
}

inline bool get_tcs4838(void)
{
	return tcs4838_exist;
}

U_BOOT_PMU_EXT_INIT(pmu_tcs4838) = {
	.pmu_ext_name	  = "pmu_tcs4838",
	.probe			  = pmu_tcs4838_probe,
	.set_dcdc_mode     = pmu_tcs4838_set_dcdc_mode,
	.get_exist	   = get_tcs4838,
};