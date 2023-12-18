/*
 * Copyright (c) 2020, Allwinner. All rights reserved.
 *
 * Author: Fan Qinghua <fanqinghua@allwinnertech.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <sunxi_cpu_ops_private.h>

void sunxi_init_archstate(unsigned int cluster, unsigned int cpu, unsigned long arch64)
{
	if (arch64) {
		mmio_setbits_32(SUNXI_INITARCH_REG(cpu), AARCH64);
	} else {
		mmio_clrbits_32(SUNXI_INITARCH_REG(cpu), AARCH64);
	}
}

void sunxi_set_bootaddr(unsigned int cluster, unsigned int cpu, uintptr_t entry)
{
	mmio_write_32(SUNXI_CPUCFG_RVBAR_LO_REG(cpu), entry);
	mmio_write_32(SUNXI_CPUCFG_RVBAR_HI_REG(cpu), 0);
}

int sunxi_get_cpu_powerstate(unsigned int cluster, unsigned int core)
{
	if ((mmio_read_32(PPU_PWSR(core + 1)) & 0xf) == STATE_ON)
		return 1;
	else
		return 0;
}

int sunxi_get_cluster_powerstate(void)
{
	if ((mmio_read_32(PPU_PWSR(0)) & 0xf) == STATE_ON)
		return 1;
	else
		return 0;
}

void sunxi_poweron_cpu(unsigned int cluster, unsigned int core)
{
	mmio_setbits_32(HOTPLUG_POWERMODE_REG(core), POWER_ON);
	mmio_setbits_32(HOTPLUG_CONTROL_REG(core), HOTPLUG_EN);

	while (!sunxi_get_cpu_powerstate(cluster, core)) {
	}

	mmio_clrbits_32(HOTPLUG_CONTROL_REG(core), HOTPLUG_EN);
	mmio_clrbits_32(HOTPLUG_POWERMODE_REG(core), POWER_ON);
}

void sunxi_poweroff_cpu(unsigned int cluster, unsigned int core)
{
	/* Set mp0_spmc_pwr_on_cpuX = 0 */
	mmio_clrbits_32(HOTPLUG_POWERMODE_REG(core), POWER_ON);
}

/*standby power off cpu0*/

/*wait for cpu power off*/
void cpucfg_cpu_suspend(void)
{
	sunxi_poweroff_cpu(0, 0);
	while (sunxi_get_cluster_powerstate()) {
	}
}
/*power off vdd_cpu*/
/*power off vdd_sys*/


/*wakeup cpu*/
/*power on vdd_sys*/
/*power on vdd_cpu*/
/*init pll_cluster*/
/*init pll_cpu*/
int cpucfg_cpu_resume(unsigned int resume_addr)
{
	/*set cpu boot addr*/
	sunxi_set_bootaddr(0, 0, resume_addr);
	/*set cpu0 to aarch64*/
	sunxi_init_archstate(0, 0, 1);
	/*power on cpu0*/
	sunxi_poweron_cpu(0, 0);

	return 0;
}
