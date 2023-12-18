/*
 * Allwinner Sun50iw10 do poweroff in uboot with arisc
 *
 * (C) Copyright 2021  <xinouyang@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <sunxi_board.h>
#include <smc.h>
#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

#ifdef CONFIG_SUNXI_PMU_EXT
#include <sunxi_power/power_manage.h>
#include <fdt_support.h>
#endif

int get_group_bit_offset(enum pin_e port_group)
{
	switch (port_group) {
	case GPIO_GROUP_B:
	case GPIO_GROUP_C:
	case GPIO_GROUP_D:
	case GPIO_GROUP_E:
	case GPIO_GROUP_F:
	case GPIO_GROUP_G:
	case GPIO_GROUP_H:
	case GPIO_GROUP_I:
	case GPIO_GROUP_J:
	case GPIO_GROUP_K:
		return port_group;
		break;
	default:
		return -1;
	}
	return -1;
}

int sunxi_platform_power_off(int status)
{
	int work_mode = get_boot_work_mode();
	/* imporve later */
	ulong cfg_base = 0;

	if (work_mode != WORK_MODE_BOOT)
		return 0;

	/* startup cpus before shutdown */
	arm_svc_arisc_startup(cfg_base);

	if (status)
		arm_svc_poweroff_charge();
	else
		arm_svc_poweroff();

	while (1) {
		asm volatile ("wfi");
	}
	return 0;
}

#ifdef CONFIG_SUNXI_PMU_EXT
static const char *const pmu_ext_reg[] = {
	"reg-tcs0", "reg-sy0", "reg-axp1530",
};

static const char *const pmu_ext[] = {
	"tcs0", "sy0", "axp1530",
};

int update_pmu_ext_info_to_kernel(void)
{
	int nodeoffset, pmu_ext_type, err, i;
	uint32_t phandle = 0;

	/* get pmu_ext type */
	pmu_ext_type = pmu_ext_get_type();
	if (pmu_ext_type < 0) {
		pr_err("Could not find pmu_ext type: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	/* get used pmu_ext node */
	nodeoffset = fdt_path_offset(working_fdt, pmu_ext_reg[pmu_ext_type]);
	if (nodeoffset < 0) {
		pr_err("Could not find nodeoffset for used ext pmu:%s\n", pmu_ext_reg[pmu_ext_type]);
		return -1;
	}
	/* get used pmu_ext phandle */
	phandle = fdt_get_phandle(working_fdt, nodeoffset);
	if (!phandle) {
		pr_err("Could not find phandle for used ext pmu:%s\n", pmu_ext_reg[pmu_ext_type]);
		return -1;
	}
	pr_debug("get ext power phandle %d\n", phandle);

	/* delete other pmu_ext node */
	for (i = 0; i < NR_PMU_EXT_VARIANTS; i++) {
		if (i == pmu_ext_type)
			continue;

		nodeoffset = fdt_path_offset(working_fdt, pmu_ext[i]);
		err = fdt_del_node(working_fdt, nodeoffset);
		if (err < 0) {
			pr_warn("WARNING: fdt_del_node can't delete %s from node %s: %s\n",
				"compatible", "status", fdt_strerror(err));
			return -1;
		}
	}

	/* get cpu@4 node */
	nodeoffset = fdt_path_offset(working_fdt, "cpu-ext");
	if (nodeoffset < 0) {
		pr_err("## error: %s: L%d\n", __func__, __LINE__);
		return -1;
	}

	/* Change cpu-supply to ext dcdc*/
	err = fdt_setprop_u32(working_fdt, nodeoffset,
				"cpu-supply", phandle);
	if (err < 0) {
		pr_warn("WARNING: fdt_setprop can't set %s from node %s: %s\n",
			"compatible", "status", fdt_strerror(err));
		return -1;
	}

	return 0;
}
#endif

int sunxi_get_crypte_type(void)
{
	int version = -1;
	u32 ver_reg_offet = 0x24;
	u32 mark = 0x7;

	version = readl(SUNXI_SYSCTRL_BASE + ver_reg_offet) & mark;
	pr_debug("crypte_type %d \n", version);
	/*A and B version use software*/
	if (version < 0x2) {
		return SUNXI_CRYPT_SOFTWARE;
	}

	return SUNXI_CRYPT_HW;
}
