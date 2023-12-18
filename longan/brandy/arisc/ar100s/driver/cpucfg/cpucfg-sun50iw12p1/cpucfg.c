/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                                cpucfg module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : cpucfg.c
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-7
* Descript: cpu config module.
* Update  : date                auther      ver     notes
*           2012-5-7 17:24:32   Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#include "../cpucfg_i.h"

static void cpucfg_counter_init(void);
volatile s32 little_endian_used;
/*
*********************************************************************************************************
*                                       INITIALIZE CPUCFG
*
* Description:  initialize cpu configure module.
*
* Arguments  :  none.
*
* Returns    :  OK if initialize cpu configure succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_init(void)
{
	cpucfg_counter_init();
	little_endian_used = 0;

	return OK;
}

/*
*********************************************************************************************************
*                                       EXIT CPUCFG
*
* Description:  exit cpu configure module.
*
* Arguments  :  none.
*
* Returns    :  OK if exit cpu configure succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_exit(void)
{
	little_endian_used = 0;
	return OK;
}

/*
*********************************************************************************************************
*                                      SET LE ADDRESS SPACE
*
* Description:  set little-endian address space.
*
* Arguments  :  start   : the start address of little-endian space.
*               end     : the end address of little-endian space.
*
* Returns    :  OK if set little-endian succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_set_little_endian_address(void *start, void *end)
{
	volatile u32 value;

	if (little_endian_used >= CPUCFG_LE_SPACE_NUMBER) {
		WRN("no space for little-endian config\n");
		return -ENOSPC;
	}

	/* config space0 as little-endian */
	value = readl(CPUS_SPACE_LE_CTRL_REG);
	value |= (0x1 << little_endian_used);
	writel(value, CPUS_SPACE_LE_CTRL_REG);

	/* config start address and end address */
	writel((volatile u32)start,
	       CPUS_SPACE_START_REG(little_endian_used));
	writel((volatile u32)end, CPUS_SPACE_END_REG(little_endian_used));

	/* the number of used little-endian space increase */
	little_endian_used++;

	/* set little-endian succeeded */
	return OK;
}

s32 cpucfg_remove_little_endian_address(void *start, void *end)
{
	int i;
	volatile u32 value;

	if (little_endian_used <= 0) {
		WRN("no space for little-endian remove\n");
		return -ENODEV;
	}

	for (i = little_endian_used; i; i--)
		if (readl(CPUS_SPACE_START_REG(i)) >> 9 == (int)start >> 9)
			break;
	INF("remove:%d\n", i);
	/* disable space0 as little-endian */
	value = readl(CPUS_SPACE_LE_CTRL_REG);
	value &= ~(0x1 << i);
	writel(value, CPUS_SPACE_LE_CTRL_REG);

	/* clean start address and end address */
	writel((volatile u32)0, CPUS_SPACE_START_REG(i));
	writel((volatile u32)0, CPUS_SPACE_END_REG(i));

	/* the number of used little-endian space decrease */
	little_endian_used--;

	/*remove little-endian succeeded */
	return OK;
}

/*
*********************************************************************************************************
*                                      SET CPU RESET STATE
*
* Description:  set the reset state of cpu.
*
* Arguments  :  cpu_num : the cpu id which we want to set reset status.
*               state   : the reset state which we want to set.
*
* Returns    :  OK if get power status succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_set_cpu_reset_state(u32 cpu_num, s32 state)
{
	volatile u32 value;

	ASSERT(cpu_num < CPUCFG_CPU_NUMBER);

	/* set cluster0 cpux state */
	value = readl(CO_CPU_CTRL_REG(cpu_num));
	value &= ~(0x1 << 0);
	value |= ((state & 0x1) << 0);
	writel(value, CO_CPU_CTRL_REG(cpu_num));

	return OK;
}

/*
*********************************************************************************************************
*                                      CLEAR 64BIT COUNTER
*
* Description:  clear 64bit counter, after this operation,
*               the value of conuter will reset to zero.
*
* Arguments  :  none.
*
* Returns    :  OK if clear counter succeeded, others if failed.
*********************************************************************************************************
*/
s32 cpucfg_counter_clear(void)
{
	writel(0, CNT_LOW_REG_SET);
	writel(0, CNT_HIGH_REG_SET);

	return OK;
}

/*
*********************************************************************************************************
*                                      READ 64BIT COUNTER
*
* Description:  read 64bit counter, the counter value base on us.
*
* Arguments  :  none.
*
* Returns    :  the value of counter, base on us.
*********************************************************************************************************
*/
u64 cpucfg_counter_read(void)
{
	volatile u32 high;
	volatile u32 low;
	u64 counter;

	do {
		low = readl(CNT_LOW_REG);
		high = readl(CNT_HIGH_REG);
	} while ((high != readl(CNT_HIGH_REG))
		 || (low > readl(CNT_LOW_REG)));

	counter = high;
	counter = (counter << 32) + low;

	return counter;
}

void cpucfg_counter_ctrl(bool enable)
{
	writel(enable, CNT_CTRL_REG);
}

static void cpucfg_counter_init(void)
{
	writel(CCU_HOSC_FREQ, CNT_FREQID_REG);
}


void cpucfg_acinactm_process(u32 status)
{
	volatile u32 value;

	/* Assert ACINACTM to LOW */
	value = readl(C0_CTRL_REG1);
	value &= ~0x1;
	value |= (0x1 & status);
	writel(value, C0_CTRL_REG1);
}

void cpucfg_wait_l2_enter_wfi(void)
{
#ifndef CFG_FPGA_PLATFORM
	while ((readl(C0_CPU_STATUS_REG) & (1 << 0)) != 1)
		;
#endif
}

void cpucfg_l1l2_reset_by_hardware(u32 status)
{
	volatile u32 value;

	/* reset L2 cache */
	value = readl(C0_CTRL_REG0);
	value &= ~0x10;
	value |= (0x10 & status);
	writel(value, C0_CTRL_REG0);

	/* reset L1 cache */
	writel((readl(CO_CPU_CTRL_REG(3)) & (~0x10)) | (0x10 & status), CO_CPU_CTRL_REG(3));
	writel((readl(CO_CPU_CTRL_REG(2)) & (~0x10)) | (0x10 & status), CO_CPU_CTRL_REG(2));
	writel((readl(CO_CPU_CTRL_REG(1)) & (~0x10)) | (0x10 & status), CO_CPU_CTRL_REG(1));
	writel((readl(CO_CPU_CTRL_REG(0)) & (~0x10)) | (0x10 & status), CO_CPU_CTRL_REG(0));
}

void cpucfg_cluster0_process(u32 status)
{
	switch (status) {
	case CLUSTER_RESET_ASSERT:
		{
			writel(readl(CO_CPU_CTRL_REG(3)) & (~0x07), CO_CPU_CTRL_REG(3));
			writel(readl(CO_CPU_CTRL_REG(2)) & (~0x07), CO_CPU_CTRL_REG(2));
			writel(readl(CO_CPU_CTRL_REG(1)) & (~0x07), CO_CPU_CTRL_REG(1));
			writel(readl(CO_CPU_CTRL_REG(0)) & (~0x07), CO_CPU_CTRL_REG(0));
			writel(0, SUNXI_CPU_RST_CTRL(0));
			break;
		}
	case CLUSTER_RESET_DEASSERT:
		{
			writel(0x13000100, SUNXI_CPU_RST_CTRL(0));
			writel(readl(CO_CPU_CTRL_REG(0)) | (0x1 << 1) \
					| (0x1 << 2) | (0x1 << 8), CO_CPU_CTRL_REG(0));

			writel(readl(CO_CPU_CTRL_REG(1)) | (0x1 << 1) \
					| (0x1 << 2) | (0x1 << 8), CO_CPU_CTRL_REG(1));

			writel(readl(CO_CPU_CTRL_REG(2)) | (0x1 << 1) \
					| (0x1 << 2) | (0x1 << 8), CO_CPU_CTRL_REG(2));

			writel(readl(CO_CPU_CTRL_REG(3)) | (0x1 << 1) \
					| (0x1 << 2) | (0x1 << 8), CO_CPU_CTRL_REG(3));
			break;
		}
	default:
		{
			WRN("invalid cluster reset status\n");
		}
	}
}

static int cpu_power_switch_set(u32 cluster, u32 cpu, bool enable)
{
	if (enable) {
		if (0x00 == readl(SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			WRN("cpu%d power switch enable already\n", cpu);
			return OK;
		}
		/* de-active cpu power clamp */
		writel(0xFE, SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		time_udelay(20);

		writel(0xF8, SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		time_udelay(10);

		writel(0xE0, SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		time_udelay(10);

		writel(0xc0, SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		time_udelay(10);

		writel(0x80, SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		time_udelay(10);

		writel(0x40, SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		time_udelay(10);

		writel(0x00, SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		time_udelay(10);

		while (0x00 != readl(SUNXI_CPU_PWR_CLAMP(cluster, cpu)))
			time_udelay(20);
		while (0x00 != readl(SUNXI_CPU_PWR_CLAMP(cluster, cpu)))
			;
	} else {
		if (0xFF == readl(SUNXI_CPU_PWR_CLAMP(cluster, cpu))) {
			WRN("cpu%d power switch disable already\n", cpu);
			return OK;
		}
		writel(0xFF, SUNXI_CPU_PWR_CLAMP(cluster, cpu));
		time_udelay(30);
		while (0xFF != readl(SUNXI_CPU_PWR_CLAMP(cluster, cpu)))
			;
	}
	return OK;
}

/*
 * set cpu die before pmu power off
 */
void cpucfg_cpu_suspend(void)
{
	volatile u32 value;

	/* Assert ACINACTM to HIGH */
	cpucfg_acinactm_process(ACINACTM_HIGH);
	save_state_flag(REC_HOTPULG | 0x0);

	/* wait L2 enter WFI */
	cpucfg_wait_l2_enter_wfi();
	save_state_flag(REC_HOTPULG | 0x1);

	value = readl(CO_CPU_CTRL_REG(0));
	value &= (~(1 << 8));
	writel(value, CO_CPU_CTRL_REG(0));

	cpucfg_set_cpu_reset_state(CPUCFG_C0_CPU0, CPU_RESET_ASSERT);

	/* set cpu0 poweron reset as assert state */
	value = readl(CPUS_C0_CPU_CFG_REG(0));
	value &= (~(0x1 << 0));
	writel(value, CPUS_C0_CPU_CFG_REG(0));

	/* set cpu0 core reset as assert state */
	cpucfg_set_cpu_reset_state(CPUCFG_C0_CPU0, CPU_RESET_ASSERT);
	cpucfg_set_cpu_reset_state(CPUCFG_C0_CPU1, CPU_RESET_ASSERT);
	cpucfg_set_cpu_reset_state(CPUCFG_C0_CPU2, CPU_RESET_ASSERT);
	cpucfg_set_cpu_reset_state(CPUCFG_C0_CPU3, CPU_RESET_ASSERT);

	save_state_flag(REC_HOTPULG | 0x2);
}

void cpucfg_cpu_suspend_late(void)
{
	cpucfg_cluster0_process(CLUSTER_RESET_ASSERT);
	save_state_flag(REC_HOTPULG | 0x3);

	/* set cpu0 H_reset as assert state */
	writel(0, SUNXI_CLUSTER_PWRON_RESET(0));
	writel(readl(CPUS_C0_CPU_CFG_REG(3)) & (~0x1), CPUS_C0_CPU_CFG_REG(3));
	writel(readl(CPUS_C0_CPU_CFG_REG(2)) & (~0x1), CPUS_C0_CPU_CFG_REG(2));
	writel(readl(CPUS_C0_CPU_CFG_REG(1)) & (~0x1), CPUS_C0_CPU_CFG_REG(1));
	writel(readl(CPUS_C0_CPU_CFG_REG(0)) & (~0x1), CPUS_C0_CPU_CFG_REG(0));

	/* set system reset to assert */
	writel(0, SUNXI_CPU_SYS_RESET);
	time_mdelay(1);

	/* set clustr0+c0_cpu0 power off gating to valid */
	ccu_set_poweroff_gating_state(PWRCTL_C0CPU0,
			      CCU_POWEROFF_GATING_VALID);
	save_state_flag(REC_HOTPULG | 0x4);

	/* power off cluster0 cpu0 */
	cpu_power_switch_set(0, 0, 0);
	save_state_flag(REC_HOTPULG | 0x5);
}

/*
 * set standby or hotplug flag before.
 */
s32 cpucfg_cpu_resume_early(u32 resume_addr)
{
	/* power on cluster0 cpu0 */
	cpu_power_switch_set(0, 0, 1);
	save_state_flag(REC_HOTPULG | 0x6);

	/* set clustr0+cpu0 power off gating to invalid */
	ccu_set_poweroff_gating_state(PWRCTL_C0CPU0,
			      CCU_POWEROFF_GATING_INVALID);
	save_state_flag(REC_HOTPULG | 0x7);
	/* set system reset to de-assert state */
	writel(1, SUNXI_CPU_SYS_RESET);

	/* set cpu0 H_reset as de-assert state */
	writel(1 << 16, SUNXI_CLUSTER_PWRON_RESET(0));
	cpucfg_set_cpu_reset_state(CPUCFG_C0_CPU0, CPU_RESET_ASSERT);
	cpucfg_cluster0_process(CLUSTER_RESET_ASSERT);
	save_state_flag(REC_HOTPULG | 0x8);

	/* enable l1&l2 reset by hardware */
	cpucfg_l1l2_reset_by_hardware(L1_EN_L2_EN);
	save_state_flag(REC_HOTPULG | 0x9);

	/* Assert ACINACTM to LOW */
	cpucfg_acinactm_process(ACINACTM_LOW);
	save_state_flag(REC_HOTPULG | 0xa);

	cpucfg_cluster0_process(CLUSTER_RESET_DEASSERT);

	set_secondary_entry(resume_addr, 0);

	/* set AA32nAA64 to AA64 */
	sun50i_set_AA32nAA64(0, 0, 1);

	save_state_flag(REC_HOTPULG | 0xb);

	return OK;
}

s32 cpucfg_cpu_resume(u32 resume_addr)
{
	u32 value;

	save_state_flag(REC_HOTPULG | 0xc);

	/* set cpu0 power-on reset as deassert state */
	value = readl(CPUS_C0_CPU_CFG_REG(0));
	value |= (0x1 << 0);
	writel(value, CPUS_C0_CPU_CFG_REG(0));

	/* set cpu0 core_reset as de-assert state. */
	cpucfg_set_cpu_reset_state(CPUCFG_C0_CPU0, CPU_RESET_DEASSERT);

	save_state_flag(REC_HOTPULG | 0xd);

	return OK;
}

void set_secondary_entry(u32 entry, u32 cpu)
{
	writel(entry, SUNXI_CPU_RVBA_L(cpu));
	writel(0, SUNXI_CPU_RVBA_H(cpu));
}

void sun50i_set_AA32nAA64(u32 cluster, u32 cpu, bool is_aa64)
{
	volatile u32 value;

	value = readl(SUB_CPU_CTRL_REG(cpu));
	value &= ~(1 << 0);
	value |= (is_aa64 << 0);
	writel(value, SUB_CPU_CTRL_REG(cpu));
}

void cpu_power_up(u32 cluster, u32 cpu)
{
	u32 value;

	/* Assert nCPUPORESET LOW */
	value = readl(CO_CPU_CTRL_REG(cpu));
	value &= (~(1 << 0));
	writel(value, CO_CPU_CTRL_REG(cpu));

	/* Assert cpu power-on reset */
	value = readl(CPUS_C0_CPU_CFG_REG(cpu));
	value &= (~(1 << 0));
	writel(value, CPUS_C0_CPU_CFG_REG(cpu));

	/* set AA32nAA64 to AA64 */
	sun50i_set_AA32nAA64(cluster, cpu, 1);

	/* hold the core output clamps */
	value = readl(CPUS_C0_CPU_CFG_REG(cpu));
	value |= (0x1 << 1);
	writel(value, CPUS_C0_CPU_CFG_REG(cpu));

	/* Apply power to the PDCPU power domain. */
	cpu_power_switch_set(cluster, cpu, 1);

	/* Release the core output clamps */
	value = readl(CPUS_C0_CPU_CFG_REG(cpu));
	value &= (~(0x1 << 1));
	writel(value, CPUS_C0_CPU_CFG_REG(cpu));

	//__asm volatile ("l.csync");
	time_udelay(1);

	/* Deassert cpu power-on reset */
	value = readl(CPUS_C0_CPU_CFG_REG(cpu));
	value |= ((1 << 0));
	writel(value, CPUS_C0_CPU_CFG_REG(cpu));

	/* Deassert core reset */
	value = readl(CO_CPU_CTRL_REG(cpu));
	value |= (1 << 0);
	writel(value, CO_CPU_CTRL_REG(cpu));

	/* Assert DBGPWRDUP HIGH */
	value = readl(CO_CPU_CTRL_REG(cpu));
	value |= (1 << 8);
	writel(value, CO_CPU_CTRL_REG(cpu));

	value = readl(CPUS_C0_CPU_CFG_REG(cpu));
	value |= (1 << 4);
	writel(value, CPUS_C0_CPU_CFG_REG(cpu));
}

void cpu_power_down(u32 cluster, u32 cpu)
{
	u32 value;

#ifdef CFG_FPGA_PLATFORM
	time_cdelay(1000000);
#else
	while (!CPU_IS_WFI_MODE(cluster, cpu))
		;
#endif
	value = readl(CPUS_C0_CPU_CFG_REG(cpu));
	value &= (~(1 << 4));
	writel(value, CPUS_C0_CPU_CFG_REG(cpu));

	/* step7: Deassert DBGPWRDUP LOW */
	value = readl(CO_CPU_CTRL_REG(cpu));
	value &= (~(1 << 8));
	writel(value, CO_CPU_CTRL_REG(cpu));

	/* step8: Activate the core output clamps */
	value = readl(CPUS_C0_CPU_CFG_REG(cpu));
	value |= (1 << 1);
	writel(value, CPUS_C0_CPU_CFG_REG(cpu));
	__asm volatile ("l.csync");
	time_udelay(1);

	/* step9: Assert nCPUPORESET LOW */
	value = readl(CO_CPU_CTRL_REG(cpu));
	value &= (~(1 << 0));
	writel(value, CO_CPU_CTRL_REG(cpu));

	/* step10: Assert cpu power-on reset */
	value = readl(CPUS_C0_CPU_CFG_REG(cpu));
	value &= (~(1 << 0));
	writel(value, CPUS_C0_CPU_CFG_REG(cpu));

	/* step11: Remove power from th e PDCPU power domain */
	cpu_power_switch_set(cluster, cpu, 0);
}
