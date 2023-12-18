/*
 * Allwinner Sun50iw10 do poweroff in uboot with arisc
 *
 * (C) Copyright 2021  <xinouyang@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <sunxi_board.h>
#include <smc.h>
#ifdef CONFIG_SUNXI_PMU_EXT
#include <sunxi_power/power_manage.h>
#include <fdt_support.h>
#endif

int sunxi_platform_power_off(int status)
{
	int work_mode = get_boot_work_mode();
	if (work_mode != WORK_MODE_BOOT)
		return 0;

	/* reset arisc */
	/* as arisc in sun50iw10 is assert in boot0 and probe in uboot,
	 * and arisc probe is in board_late_init,
	 * and when using dragon sn , it won't run board_late_init,
	 * so have to reset arisc before poweroff*/
	int reg_val;
	reg_val = readl(SUNXI_R_CPUCFG_BASE);
	reg_val &= ~1;
	writel(reg_val, SUNXI_R_CPUCFG_BASE);
	reg_val |= 1;
	writel(reg_val, SUNXI_R_CPUCFG_BASE);
	/* call uboot poweroff */
	sunxi_arisc_probe();
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
	"reg_tcs0", "reg_sy0",
};

static const char *const pmu_ext[] = {
	"tcs0", "sy0",
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

	/* get cpu@0 node */
	nodeoffset = fdt_path_offset(working_fdt, "/cpus/cpu@0");
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

int update_no_ext_info_to_kernel(void)
{
	int nodeoffset, err;

	/* get cpu-opp-table node */
	nodeoffset = fdt_path_offset(working_fdt, "/cpu-opp-table/opp@1800000000");
	if (nodeoffset < 0) {
		pr_err("## error: %s\n", __func__);
		return -1;
	}

	/* delete opp@1800000000 node */
	err = fdt_del_node(working_fdt, nodeoffset);
	if (err < 0) {
		pr_warn("WARNING: fdt_del_node can't delete %s from node %s: %s\n",
			"compatible", "status", fdt_strerror(err));
		return -1;
	}

	return 0;
}
#endif

