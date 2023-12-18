/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                               standby module
*
*                                    (c) Copyright 2012-2016, superm China
*                                             All Rights Reserved
*
* File    : extended_super_standby.c
* By      : superm
* Version : v1.0
* Date    : 2013-1-16
* Descript: extended super-standby module public header.
* Update  : date                auther      ver     notes
*           2013-1-16 17:04:28  superm       1.0     Create this file.
*********************************************************************************************************
*/
#include <libfdt.h>
#include "../standby_i.h"
#include "wakeup_source.h"
#include "cpucfg_regs.h"

extern u32 dram_crc_enable;
extern u32 dram_crc_src;
extern u32 dram_crc_len;
extern u32 volatile wakeup_source;
extern u32 axp_power_max;
extern u32 dtb_base;

static s32 result;
static u32 suspend_lock;
static u32 standby_type;

/* standby paras */
static uint32_t standby_vdd_cpu;
static uint32_t standby_vdd_cpub;
static uint32_t standby_vdd_sys;
static uint32_t standby_vcc_pll;
static uint32_t standby_vcc_io;
static uint32_t standby_osc24m_on = 1;

static void dcxo_disable(void)
{
}

static void dcxo_enable(void)
{
}

static s32 standby_dts_parse(void)
{
	static u32 dts_has_parsed;
	void *fdt;
	int32_t param_node;

	if (!!dts_has_parsed)
		return 0;

	fdt = (void *)(dtb_base);

    /* parse power tree */
    param_node = fdt_path_offset(fdt, "standby_param");
    if (param_node < 0) {
		WRN("no standby_param: %x fdt:%x\n", param_node, fdt);
		return -1;
    }

	fdt_getprop_u32(fdt, param_node, "vdd-cpu", &standby_vdd_cpu);
	fdt_getprop_u32(fdt, param_node, "vdd-cpub", &standby_vdd_cpub);
	fdt_getprop_u32(fdt, param_node, "vdd-sys", &standby_vdd_sys);
	fdt_getprop_u32(fdt, param_node, "vcc-pll", &standby_vcc_pll);
	fdt_getprop_u32(fdt, param_node, "vcc-io", &standby_vcc_io);
	fdt_getprop_u32(fdt, param_node, "osc24m-on", &standby_osc24m_on);
	LOG("standby power %x, %x, %x, %x, %x\n",
			standby_vdd_cpu, standby_vdd_sys, standby_vcc_pll, standby_vcc_io, standby_osc24m_on);

	dts_has_parsed = 1;

	return 0;
}

static void wait_wakeup(void)
{
	save_state_flag(REC_ESTANDBY | REC_WAIT_WAKEUP | 0x01);
	wakeup_timer_start();
	while (1) {
		/*
		 * maybe add user defined task process here
		 */


		if (wakeup_source != NO_WAKESOURCE) {
			LOG("wakeup: %d\n", wakeup_source);
			break;
		}

		/* FIXME later */
		writel(readl(LP_CTRL_REG) | ((1 << 24) | (1 << 25) | (1 << 26) | (1 << 27)), LP_CTRL_REG);
		cpu_enter_doze();
		writel(readl(LP_CTRL_REG) & (~((1 << 24) | (1 << 25) | (1 << 26) | (1 << 27))), LP_CTRL_REG);
	}
	wakeup_timer_stop();
}

static void wait_cpu0_resume(void)
{
	s32 ret;
	struct message message;

	/* no paras for resume notify message */
	message.paras = NULL;

	printk("wait ac327 resume...\n");

	/* wait cpu0 restore finished. */
	while (1) {
		ret = hwmsgbox_query_message(&message, 0);
		if (ret != OK)
			continue; /* no message, query again */

		/* query valid message */
		if (message.type == SSTANDBY_RESTORE_NOTIFY) {
			/* cpu0 restore, feedback wakeup event. */
			LOG("cpu0 restore finished\n");
			/* init feedback message */
			message.count = 1;
			message.paras = (u32 *)&wakeup_source;
			/* synchronous message, need feedback. */
			hwmsgbox_feedback_message(&message, SEND_MSG_TIMEOUT);
			break;
		} else {
			/* invalid message detected, ignore it, by sunny at 2012-6-28 11:33:13. */
			ERR("standby ignore message [%x]\n", message.type);
		}
	}

	wakeup_source = NO_WAKESOURCE;
}

static void dm_suspend(void)
{
}

static void dm_resume(void)
{
}

static void dram_suspend(void)
{
}

static void dram_resume(void)
{
}

static void smc_standby_init(void)
{
}

static void smc_standby_exit(void)
{
}

static void device_suspend(void)
{
	smc_standby_init();
	pmu_standby_init();
	twi_standby_init();
	hwmsgbox_super_standby_init();
}

static void device_resume(void)
{
	hwmsgbox_super_standby_exit();
	twi_standby_exit();
	pmu_standby_exit();
	smc_standby_exit();
}

static void clk_suspend(void)
{
}

static void clk_suspend_late(void)
{
	dcxo_disable();
}

static void clk_resume_early(void)
{
	dcxo_enable();
}

static void clk_resume(void)
{
}

static void system_suspend(void)
{
}

static void system_resume(void)
{
}

static u32 platform_standby_type(void)
{
	u32 type = 0;

	/* usb standby */
	if (interrupt_get_enabled(INTC_R_USB_IRQ)) {
		type |= CPUS_WAKEUP_USB;
	}

	return type;
}

static s32 standby_process_init(struct message *pmessage)
{
	suspend_lock = 1;

	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x1);

	cpucfg_cpu_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x2);

	device_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x3);

	dram_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x4);

//	cpucfg_cpu_suspend_late();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x5);

	clk_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x6);

	system_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x7);

	clk_suspend_late();

	dm_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x8);

	return OK;
}

static s32 standby_process_exit(struct message *pmessage)
{
	u32 resume_entry = pmessage->paras[1];

	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x1);

	dm_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x2);

	clk_resume_early();

	system_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x3);

	clk_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x4);

//	cpucfg_cpu_resume_early(resume_entry);
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x5);

	dram_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x6);

	device_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x7);

	cpucfg_cpu_resume(resume_entry);
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x8);

	wait_cpu0_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x9);

	suspend_lock = 0;

	return OK;
}

/*
*********************************************************************************************************
*                                       ENTEY OF TALK-STANDBY
*
* Description:  the entry of extended super-standby.
*
* Arguments  :  request:request command message.
*
* Returns    :  OK if enter extended super-standby succeeded, others if failed.
*********************************************************************************************************
*/
static s32 standby_entry(struct message *pmessage)
{
	save_state_flag(REC_ESTANDBY | REC_ENTER);

	standby_dts_parse();

	save_state_flag(REC_ESTANDBY | REC_ENTER | 0x1);

	/* backup cpus source clock */
	iosc_freq_init();

	/* parse standby type from enabled interrupt */
	standby_type = platform_standby_type();

	/*
	 * --------------------------------------------------------------------------
	 *
	 * initialize enter super-standby porcess
	 *
	 * --------------------------------------------------------------------------
	 */
	save_state_flag(REC_ESTANDBY | REC_BEFORE_INIT);
	result = standby_process_init(pmessage);
	save_state_flag(REC_ESTANDBY | REC_AFTER_INIT);

	/*
	 * --------------------------------------------------------------------------
	 *
	 * wait valid wakeup source porcess
	 *
	 * --------------------------------------------------------------------------
	 */
	LOG("wait wakeup\n");
	save_state_flag(REC_ESTANDBY | REC_WAIT_WAKEUP);

	wait_wakeup();

	/*
	 * --------------------------------------------------------------------------
	 *
	 * exit super-standby wakeup porcess
	 *
	 * --------------------------------------------------------------------------
	 */
	save_state_flag(REC_ESTANDBY | REC_BEFORE_EXIT);
	standby_process_exit(pmessage);
	save_state_flag(REC_ESTANDBY | REC_AFTER_EXIT);

	return OK;
}

u32 is_suspend_lock(void)
{
	return suspend_lock;
}

int cpu_op(struct message *pmessage)
{
	u32 mpidr = pmessage->paras[0];
	u32 entrypoint = pmessage->paras[1];
	u32 cpu_state = pmessage->paras[2];
	u32 cluster_state = pmessage->paras[3]; /* unused variable */
	u32 system_state = pmessage->paras[4];

	LOG("mpidr:%x, entrypoint:%x; cpu_state:%x, cluster_state:%x, system_state:%x\n", mpidr, entrypoint, cpu_state, cluster_state, system_state);
	if (entrypoint && system_state == arisc_power_off) {
		standby_entry(pmessage);
	}

	return 0;
}

static void system_shutdown(void)
{
	pmu_shutdown();
}

static void system_reset(void)
{
	pmu_reset();
}

int sys_op(struct message *pmessage)
{
	u32 state = pmessage->paras[0];

	LOG("state:%x\n", state);

	switch (state) {
	case arisc_system_shutdown:
		{
			save_state_flag(REC_SHUTDOWN | 0x101);
			pmu_charging_reset();
			system_shutdown();
			break;
		}
	case arisc_system_reset:
	case arisc_system_reboot:
		{
			save_state_flag(REC_SHUTDOWN | 0x102);
			system_reset();
			break;
		}
	case arisc_uboot_shutdown:
		{
			save_state_flag(REC_SHUTDOWN | 0x103);
			system_shutdown();
			break;
		}
	default:
		{
			WRN("invaid system power state (%d)\n", state);
			return -EINVAL;
		}
	}

	return 0;
}

s32 fake_poweroff(struct message *pmessage)
{
	return 0;
}

