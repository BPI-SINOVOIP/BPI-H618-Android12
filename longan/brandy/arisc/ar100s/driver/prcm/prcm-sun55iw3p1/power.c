/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                         clock control unit module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : power.c
* By      : Sunny
* Version : v1.0
* Date    : 2012-11-22
* Descript: module power manager.
* Update  : date                auther      ver     notes
*           2012-11-22 16:44:06 Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#include "ccu_i.h"
#include "cpucfg_regs.h"

static u32 hosc_lock;
static u32 pll_ldo_suspend;
/*
*********************************************************************************************************
*                                    SET POWER OFF STATUS OF HWMODULE
*
* Description:  set the power off gating status of a specific module.
*
* Arguments  :  module  : the module ID which we want to set power off gating status.
*               state   : the power off status which we want to set, the detail please
*                         refer to the status of power-off gating.
*
* Returns    :  OK if set module power off gating status succeeded, others if failed.
*********************************************************************************************************
*/
s32 ccu_set_poweroff_gating_state(s32 module, s32 state)
{
	volatile u32 value;

	switch (module) {
	case PWRCTL_C0CPU0:
		{
			value = readl(CPUS_C0_CPU_CFG_REG(0));
			value &= (~(0x1 << 1));
			value |= (state & 0x1) << 1;
			writel(value, CPUS_C0_CPU_CFG_REG(0));
			return OK;
		}
	case PWRCTL_C0CPU1:
		{
			value = readl(CPUS_C0_CPU_CFG_REG(1));
			value &= (~(0x1 << 1));
			value |= (state & 0x1) << 1;
			writel(value, CPUS_C0_CPU_CFG_REG(1));
			return OK;
		}
	case PWRCTL_C0CPU2:
		{
			value = readl(CPUS_C0_CPU_CFG_REG(2));
			value &= (~(0x1 << 1));
			value |= (state & 0x1) << 1;
			writel(value, CPUS_C0_CPU_CFG_REG(2));
			return OK;
		}
	case PWRCTL_C0CPU3:
		{
			value = readl(CPUS_C0_CPU_CFG_REG(3));
			value &= (~(0x1 << 1));
			value |= (state & 0x1) << 1;
			writel(value, CPUS_C0_CPU_CFG_REG(3));
			return OK;
		}
	case PWRCTL_C0CPUX:
		{
			value = readl(C0_CPUX_POWEROFF_GATING_REG);
			value &= (~(0x1 << 4));
			value |= (state & 0x1) << 4;
			writel(value, C0_CPUX_POWEROFF_GATING_REG);
			return OK;
		}
	default:
		{
			WRN("invaid power control module (%d) when set power-off gating\n", module);
			return -EINVAL;
		}
	}
	/* un-reached */
}

struct notifier *hosc_notifier_list;

s32 ccu_24mhosc_reg_cb(__pNotifier_t pcb)
{
	/* insert call-back to hosc_notifier_list. */
	return notifier_insert(&hosc_notifier_list, pcb);
}

s32 ccu_24mhosc_disable(void)
{
	u32 value = 0;

	hosc_lock = 1;

	/* notify 24mhosc will power-off */
	INF("broadcast 24mhosc will power-off\n");
	notifier_notify(&hosc_notifier_list, CCU_HOSC_WILL_OFF_NOTIFY, 0);

	/* power-off pll ldo */
	pll_ldo_suspend = (readl(PLL_CTRL_REG1) >> 16) & 0x7;
	value = (readl(PLL_CTRL_REG1) | (0xa7 << 24));
	writel(value, PLL_CTRL_REG1);
	value = (readl(PLL_CTRL_REG1) | (0xa7 << 24));
	value &= (~(0x1 << 0));
	writel(value, PLL_CTRL_REG1);

	time_cdelay(20);

	/* power-off dcxo */
	value = 0x16aa;
	writel(value, RTC_XO_WRT_PROTECT);
	value = readl(RTC_XO_CTRL_REG);
	value &= (~(0x1 << 1));
	writel(value, RTC_XO_CTRL_REG);

	return OK;
}

s32 ccu_24mhosc_enable(void)
{
	u32 value = 0;

	/* power-on dcxo */
	value = 0x16aa;
	writel(value, RTC_XO_WRT_PROTECT);
	value = readl(RTC_XO_CTRL_REG);
	value &= (~(0x1 << 1));
	value |= (0x1 << 1);
	writel(value, RTC_XO_CTRL_REG);

	/* NOTICE: we must wait 4ms for dcxo enable!! */
	time_mdelay(4);

	/* power-on pll ldo */
	value = (readl(PLL_CTRL_REG1) | (0xa7 << 24));
	writel(value, PLL_CTRL_REG1);
	value = (readl(PLL_CTRL_REG1) | (0xa7 << 24));
	value |= (0x1 << 0);
	writel(value, PLL_CTRL_REG1);
	writel(value, PLL_CTRL_REG1);

	/* wait 1ms for 24m hosc ready */
	time_mdelay(1);

#if 0
	/* adjust pll voltage to 1.45v */
	value = (readl(PLL_CTRL_REG1) | (0xa7 << 24));
	writel(value, PLL_CTRL_REG1);
	value = (readl(PLL_CTRL_REG1) | (0xa7 << 24));
	value &= (~(0x7 << 16));
	value |= (pll_ldo_suspend << 16);
	writel(value, PLL_CTRL_REG1);

	/* wait 2ms for voltage ready */
	time_mdelay(2);
#endif

	/* notify 24mhosc power-on ready */
	INF("broadcast 24mhosc power-on ready\n");
	notifier_notify(&hosc_notifier_list, CCU_HOSC_ON_READY_NOTIFY, 0);

	hosc_lock = 0;

	return OK;
}

s32 is_hosc_lock(void)
{
	return hosc_lock;
}
