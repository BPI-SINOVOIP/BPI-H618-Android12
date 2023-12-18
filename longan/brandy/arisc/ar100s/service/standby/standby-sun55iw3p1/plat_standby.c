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
extern u32 ic_version_a;

static s32 result;
static u32 suspend_lock;
static u32 standby_type;
static u32 usb_standby_port;
static u32 pll_periph0, mbus;

/* standby paras */
static uint32_t standby_vdd_cpu;
static uint32_t standby_vdd_cpub;
static uint32_t standby_vdd_ndr;
static uint32_t standby_vdd_sys;
static uint32_t standby_vcc_pll;
static uint32_t standby_vcc_io;
static uint32_t standby_osc24m_on = 1;

/* pmu ext check */
static uint32_t pmu_ext_power_max = -1;

/* backup PLL */
typedef struct PLL_PAMA {
	u32 addr;
	u8  pll_en;
	u8  pll_ldo_en;
	u8  lock_en;
	u8  out_put_en;
	u32 factor;
} rPLL_PAMA;
rPLL_PAMA pll_restore[11];

/* backup cpu PLL */
static u32 cpu_pll_restore_bak[4] = {0};
static u32 cpu_ssc_restore_bak[4] = {0};
static u32 cpu_clk_restore_bak[3] = {0};

/*  backup cpu/cpus source clock */
typedef struct BUS_PAMA {
	u32 busAddr;
	u8  bus_clk_src;
	u8  M;
	u8  flag;//check the first write is or not default;
} rBUS_PAMA;
rBUS_PAMA bus_restore[5];

enum pll_state {
	pll_disable = 0,
	pll_enable  = 1
};

enum bus_clock_mode {
	osc_24M = 0,
	rtc_32k = 1,
	rc16m  = 2,
	pll_peri0 = 3,
	clock_bak = 4
};

static void suspend_para_prepare(void)
{
	static u32 para_has_parsed;

	if (!!para_has_parsed)
		return;

	/* set up restore paras */
	pll_restore[0].addr = CCU_PLL_DDR0_REG;
	pll_restore[1].addr = CCU_PLL_PERIPH0_REG;
	pll_restore[2].addr = CCU_PLL_PERIPH1_REG;
	pll_restore[3].addr = CCU_PLL_GPU_REG;
	pll_restore[4].addr = CCU_PLL_VIDEO0_REG;
	pll_restore[5].addr = CCU_PLL_VIDEO1_REG;
	pll_restore[6].addr = CCU_PLL_VIDEO2_REG;
	pll_restore[7].addr = CCU_PLL_VE_REG;
	pll_restore[8].addr = CCU_PLL_VIDEO3_REG;
	pll_restore[9].addr = CCU_PLL_AUDIO_REG;
	pll_restore[10].addr = CCU_PLL_NPU_REG;

	bus_restore[0].busAddr = CCU_AHB_CFG_REG;
	bus_restore[1].busAddr = CCU_APB0_CFG_REG;
	bus_restore[2].busAddr = CCU_APB1_CFG_REG;
	bus_restore[3].busAddr = CPUS_CFG_REG;
	bus_restore[4].busAddr = APBS0_CFG_REG;
	/* apbs1 need separate recovery */

	/* get pmu_ext type */
	pmu_ext_power_max = pmu_ext_is_exist();
	para_has_parsed = 1;
}

static void dcxo_disable(void)
{
	u32 val;

	val = readl(RTC_XO_CTRL_REG) & (1 << 31);
	if ((!standby_osc24m_on) || (!!val)) {
		ccu_24mhosc_disable();
	}
}

static void dcxo_enable(void)
{
	u32 val;

	val = readl(RTC_XO_CTRL_REG) & (1 << 31);
	if ((!standby_osc24m_on) || (!!val)) {
		ccu_24mhosc_enable();
		time_mdelay(1);
	}
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
    param_node = fdt_path_offset(fdt, "standby-param");
    if (param_node < 0) {
		WRN("no standby-param: %x fdt:%x\n", param_node, fdt);
		return -1;
    }

	fdt_getprop_u32(fdt, param_node, "vdd-cpu", &standby_vdd_cpu);
	fdt_getprop_u32(fdt, param_node, "vdd-cpub", &standby_vdd_cpub);
	fdt_getprop_u32(fdt, param_node, "vdd-ndr", &standby_vdd_ndr);
	fdt_getprop_u32(fdt, param_node, "vdd-sys", &standby_vdd_sys);
	fdt_getprop_u32(fdt, param_node, "vcc-pll", &standby_vcc_pll);
	fdt_getprop_u32(fdt, param_node, "vcc-io", &standby_vcc_io);
	fdt_getprop_u32(fdt, param_node, "osc24m-on", &standby_osc24m_on);
	printk("standby power %x, %x, %x, %x, %x, %x, %x\n",
			standby_vdd_cpu, standby_vdd_cpub, standby_vdd_ndr, standby_vdd_sys, standby_vcc_pll, standby_vcc_io, standby_osc24m_on);

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
			message.result = 0;
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
	u32 type;

	if (ic_version_a)
		return;

	/* vdd-ndr powerdown */
	for (type = 0; type < pmu_ext_power_max; type++) {
		if ((standby_vdd_ndr >> type) & 0x1)
			pmu_ext_set_voltage_state(type, POWER_VOL_OFF);
	}

	/* vdd-cpub powerdown */
	for (type = 0; type < pmu_ext_power_max; type++) {
		if ((standby_vdd_cpub >> type) & 0x1)
			pmu_ext_set_voltage_state(type, POWER_VOL_OFF);
	}

	/* vdd-io powerdown */
	for (type = 0; type < axp_power_max; type++) {
		if ((standby_vcc_io >> type) & 0x1)
			pmu_set_voltage_state(type, POWER_VOL_OFF);
	}

	/* vdd-cpu powerdown */
	for (type = 0; type < axp_power_max; type++) {
		if ((standby_vdd_cpu >> type) & 0x1)
			pmu_set_voltage_state(type, POWER_VOL_OFF);
	}

	/* vdd-sys powerdown */
	for (type = 0; type < axp_power_max; type++) {
		if ((standby_vdd_sys >> type) & 0x1)
			pmu_set_voltage_state(type, POWER_VOL_OFF);
	}

	/* vcc-pll powerdown */
	for (type = 0; type < axp_power_max; type++) {
		if ((standby_vcc_pll >> type) & 0x1)
			pmu_set_voltage_state(type, POWER_VOL_OFF);
	}
}

static void dm_resume(void)
{
	u32 type;

	if (ic_version_a)
		return;

	/* vcc-pll powerup */
	for (type = 0; type < axp_power_max; type++) {
		if ((standby_vcc_pll >> type) & 0x1)
			pmu_set_voltage_state(type, POWER_VOL_ON);
	}

	/* vdd-sys powerup */
	for (type = 0; type < axp_power_max; type++) {
		if ((standby_vdd_sys >> type) & 0x1)
			pmu_set_voltage_state(type, POWER_VOL_ON);
	}

	/* vdd-cpu powerup */
	for (type = 0; type < axp_power_max; type++) {
		if ((standby_vdd_cpu >> type) & 0x1)
			pmu_set_voltage_state(type, POWER_VOL_ON);
	}

	/* vdd-io powerup */
	for (type = 0; type < axp_power_max; type++) {
		if ((standby_vcc_io >> type) & 0x1)
			pmu_set_voltage_state(type, POWER_VOL_ON);
	}

	/* vdd-cpub powerup */
	for (type = 0; type < pmu_ext_power_max; type++) {
		if ((standby_vdd_cpub >> type) & 0x1)
			pmu_ext_set_voltage_state(type, POWER_VOL_ON);
	}

	/* vdd-ndr powerup */
	for (type = 0; type < pmu_ext_power_max; type++) {
		if ((standby_vdd_ndr >> type) & 0x1)
			pmu_ext_set_voltage_state(type, POWER_VOL_ON);
	}
}

static void dram_suspend(void)
{
#ifndef CFG_FPGA_PLATFORM
	/* calc dram checksum */
	if (standby_dram_crc_enable()) {
		before_crc = standby_dram_crc();
		LOG("before_crc: 0x%x\n", before_crc);
	}
#endif
	pll_periph0 = readl(CCU_PLL_PERIPH0_REG);
	mbus = readl(CCU_MBUS_CLK_REG);
	dram_power_save_process(&arisc_para.dram_para);
	/* DDRIO set */
	writel((readl(VDD_SYS_PWROFF_GATING_REG) & (~VDD_DDR_GATING_MASK)) | VDD_DDR_GATING(0), VDD_SYS_PWROFF_GATING_REG);
}

static void dram_resume(void)
{
	u32 reg_val;
	/* restore dram controller and transing area. */
	LOG("power-up dram\n");

	/*
	 * mbus default clk src is 24MHz, switch to pll_periph0(x2),
	 * so before increase mbus freq, should set div firstly.
	 * by Superm Wu at 2015-09-18
	 */
#ifndef CFG_FPGA_PLATFORM
	reg_val = readl(CCU_MBUS_CLK_REG);
	reg_val |= mbus & MBUS_FACTOR_M_MASK;
	reg_val |= MBUS_UPDATE_MASK;
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(200);

	reg_val = readl(CCU_MBUS_CLK_REG);
	reg_val |= mbus & MBUS_CLK_SRC_SEL_MASK;
	reg_val |= MBUS_UPDATE_MASK;
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(20);

	reg_val = readl(CCU_MBUS_CLK_REG);
	reg_val |= 0x1 << 30;
	reg_val |= MBUS_UPDATE_MASK;
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(20);

	reg_val = readl(CCU_MBUS_CLK_REG);
	reg_val |= 0x1 << 31;
	reg_val |= mbus & MBUS_UPDATE_MASK;
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(20);

	while ((readl(CCU_MBUS_CLK_REG) & MBUS_UPDATE_MASK))
		;

	dram_power_up_process(&arisc_para.dram_para);

	/* calc dram checksum */
	if (standby_dram_crc_enable()) {
//		dram_master_enable(16, 1);
		after_crc = standby_dram_crc();
		if (after_crc != before_crc) {
			save_state_flag(REC_SSTANDBY | REC_DRAM_DBG | 0xf);
			ERR("dram crc error...\n");
			ERR("---->>>>LOOP<<<<----\n");
			while (1)
				;
		}
	}
#else
	/* on fpga platform, do nothing */
#endif
}

struct SMC_REGION_T {
	u32 region_low;
	u32 region_high;
	u32 region_attr;
} static smc_region_save[SMC_REGION_COUNT];

static void smc_standby_init(void)
{
	int read_idx;
	for (read_idx = 0; read_idx < SMC_REGION_COUNT; read_idx++) {
		if (readl(SMC_REGIN_ATTRIBUTE_REG(read_idx)) == 0)
			break;
		smc_region_save[read_idx].region_low =
		    readl(SMC_REGIN_SETUP_LOW_REG(read_idx));
		smc_region_save[read_idx].region_high =
		    readl(SMC_REGIN_SETUP_HIGH_REG(read_idx));
		smc_region_save[read_idx].region_attr =
		    readl(SMC_REGIN_ATTRIBUTE_REG(read_idx));
	}
}

static void smc_standby_exit(void)
{
	int read_idx, resume_idx;
	if ((readl(SID_SEC_MODE_STA) & SID_SEC_MODE_MASK) == 0) {
		/*chip non-secure, do not need smc config*/
		return;
	}

	/*enable smc control*/
	writel(0x0, SMC_ACTION_REG);
	writel(0, SMC_MST0_BYP_REG);
	writel(0, SMC_MST1_BYP_REG);
	writel(0xffffffff, SMC_MST0_SEC_REG);
	writel(0xffffffff, SMC_MST1_SEC_REG);

	/*resume region settings*/
	resume_idx = 0;
	for (read_idx = 0; read_idx < SMC_REGION_COUNT; read_idx++) {
		if (smc_region_save[read_idx].region_attr == 0)
			break;
		writel(smc_region_save[resume_idx].region_low,
		       SMC_REGIN_SETUP_LOW_REG(resume_idx));
		writel(smc_region_save[resume_idx].region_high,
		       SMC_REGIN_SETUP_HIGH_REG(resume_idx));
		writel(smc_region_save[resume_idx].region_attr,
		       SMC_REGIN_ATTRIBUTE_REG(resume_idx));
		resume_idx++;
	}
}

static u32 usb_standby_port_support(void)
{
	u32 port = 0, val;

	val = readl(USB0_OHCI_CTRL_REG);
	LOG("[%s] 0x%x:0x%x [%x]\n", __func__, USB0_OHCI_CTRL_REG, readl(USB0_OHCI_CTRL_REG), val & OHCI_USB_SUSPEND);
	if ((val & OHCI_CTRL_HCFS) == OHCI_USB_SUSPEND)
		port |= 0x1; /* usb0 support usb standby */

	val = readl(USB1_OHCI_CTRL_REG);
	LOG("[%s] 0x%x:0x%x [%x]\n", __func__, USB1_OHCI_CTRL_REG, readl(USB1_OHCI_CTRL_REG), val & OHCI_USB_SUSPEND);
	if ((val & OHCI_CTRL_HCFS) == OHCI_USB_SUSPEND)
		port |= 0x2; /* usb1 support usb standby */

	return port;
}

static void usb_standby_init(void)
{
	/* parse which port support usb standby from HCFS */
	usb_standby_port = usb_standby_port_support();
}

static void usb_standby_exit(void)
{
	/* restore all ports support usb standby to zero */
	usb_standby_port = 0;
}

static void usb_iso_suspend(void)
{
	u32 val;
	u8 usb0_en = usb_standby_port & 0x1 ? 1 : 0; /* usb0 support usb standby ? */
	u8 usb1_en = usb_standby_port & 0x2 ? 1 : 0; /* usb1 support usb standby ? */
	u8 usb_en  = usb_standby_port ? 1 : 0;

	LOG("[%s] standby_type :0x%x\n", __func__, standby_type);
	LOG("usb0:%d, usb1:%d, usb:%d, usb_standby_port:0x%x\n",
		usb0_en, usb1_en, usb_en, usb_standby_port);
	if (!(standby_type & CPUS_WAKEUP_USB)) {
		val = readl(VDD_SYS_PWROFF_GATING_REG);
		val |= VDD_SYS2USB_GATING(1);
		val |= VDD_USB2CPUS_GATING(1);
		writel(val, VDD_SYS_PWROFF_GATING_REG);
	} else {
		/* to do */

		if (usb0_en) {
			val = readl(CCU_USB0_CLOCK_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB0_CLOCK_REG, val);
			val &= ~SCLK_GATING_OHCI0_MASK;
			writel(val, CCU_USB0_CLOCK_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB0_CLOCK_REG, readl(CCU_USB0_CLOCK_REG));

			val = readl(CCU_USB_BUS_GATING_RST_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, val);
			val &= ~USBOHCI0_GATING_MASK;
			val &= ~USBEHCI0_GATING_MASK;
			writel(val, CCU_USB_BUS_GATING_RST_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, readl(CCU_USB_BUS_GATING_RST_REG));
		}

		if (usb1_en) {
			val = readl(CCU_USB1_CLOCK_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB1_CLOCK_REG, val);
			val &= ~SCLK_GATING_OHCI1_MASK;
			writel(val, CCU_USB1_CLOCK_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB1_CLOCK_REG, readl(CCU_USB1_CLOCK_REG));

			val = readl(CCU_USB_BUS_GATING_RST_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, val);
			val &= ~USBOHCI1_GATING_MASK;
			val &= ~USBEHCI1_GATING_MASK;
			writel(val, CCU_USB_BUS_GATING_RST_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, readl(CCU_USB_BUS_GATING_RST_REG));
		}

		if (usb_en) {
			val = readl(CCU_CLK24M_GATE_EN_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_CLK24M_GATE_EN_REG, val);
			val &= ~USB24M_GATE_EN_MASK;
			writel(val, CCU_CLK24M_GATE_EN_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_CLK24M_GATE_EN_REG, readl(CCU_CLK24M_GATE_EN_REG));
		}

		/* The isolation is necessary */
		val = readl(VDD_SYS_PWROFF_GATING_REG);
		LOG("[0] 0x%x: 0x%x\n", VDD_SYS_PWROFF_GATING_REG, val);
		val |= VDD_SYS2USB_GATING(1);
		writel(val, VDD_SYS_PWROFF_GATING_REG);
		LOG("[1] 0x%x: 0x%x\n", VDD_SYS_PWROFF_GATING_REG, readl(VDD_SYS_PWROFF_GATING_REG));

		if (usb0_en) {
			val = readl(CCU_USB0_CLOCK_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB0_CLOCK_REG, val);
			val &= ~USBPHY0_RST_MASK;
			writel(val, CCU_USB0_CLOCK_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB0_CLOCK_REG, readl(CCU_USB0_CLOCK_REG));

			val = readl(CCU_USB_BUS_GATING_RST_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, val);
			val &= ~USBEHCI0_RST_MASK;
			val &= ~USBOHCI0_RST_MASK;
			writel(val, CCU_USB_BUS_GATING_RST_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, readl(CCU_USB_BUS_GATING_RST_REG));
		}

		if (usb1_en) {
			val = readl(CCU_USB1_CLOCK_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB1_CLOCK_REG, val);
			val &= ~USBPHY1_RST_MASK;
			writel(val, CCU_USB1_CLOCK_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB1_CLOCK_REG, readl(CCU_USB1_CLOCK_REG));

			val = readl(CCU_USB_BUS_GATING_RST_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, val);
			val &= ~USBEHCI1_RST_MASK;
			val &= ~USBOHCI1_RST_MASK;
			writel(val, CCU_USB_BUS_GATING_RST_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, readl(CCU_USB_BUS_GATING_RST_REG));
		}
	}
}

static void usb_iso_resume(void)
{
	u32 val;
	u8 usb0_en = usb_standby_port & 0x1 ? 1 : 0; /* usb0 support usb standby ? */
	u8 usb1_en = usb_standby_port & 0x2 ? 1 : 0; /* usb1 support usb standby ? */
	u8 usb_en  = usb_standby_port ? 1 : 0;

	LOG("[%s] standby_type :0x%x\n", __func__, standby_type);
	LOG("usb0:%d, usb1:%d, usb:%d, usb_standby_port:0x%x\n",
		usb0_en, usb1_en, usb_en, usb_standby_port);
	if (!(standby_type & CPUS_WAKEUP_USB)) {
		val = readl(VDD_SYS_PWROFF_GATING_REG);
		val &= ~VDD_SYS2USB_GATING(1);
		val &= ~VDD_USB2CPUS_GATING(1);
		writel(val, VDD_SYS_PWROFF_GATING_REG);
	} else {
		/* to do */

		if (usb0_en) {
			val = readl(CCU_USB_BUS_GATING_RST_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, val);
			val |= USBEHCI0_RST_MASK;
			val |= USBOHCI0_RST_MASK;
			writel(val, CCU_USB_BUS_GATING_RST_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, readl(CCU_USB_BUS_GATING_RST_REG));

			val = readl(CCU_USB0_CLOCK_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB0_CLOCK_REG, val);
			val |= USBPHY0_RST_MASK;
			writel(val, CCU_USB0_CLOCK_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB0_CLOCK_REG, readl(CCU_USB0_CLOCK_REG));
		}

		if (usb1_en) {
			val = readl(CCU_USB_BUS_GATING_RST_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, val);
			val |= USBEHCI1_RST_MASK;
			val |= USBOHCI1_RST_MASK;
			writel(val, CCU_USB_BUS_GATING_RST_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, readl(CCU_USB_BUS_GATING_RST_REG));

			val = readl(CCU_USB1_CLOCK_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB1_CLOCK_REG, val);
			val |= USBPHY1_RST_MASK;
			writel(val, CCU_USB1_CLOCK_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB1_CLOCK_REG, readl(CCU_USB1_CLOCK_REG));
		}

		/* The isolation is necessary */
		val = readl(VDD_SYS_PWROFF_GATING_REG);
		LOG("[0] 0x%x: 0x%x\n", VDD_SYS_PWROFF_GATING_REG, val);
		val &= ~VDD_SYS2USB_GATING(1);
		writel(val, VDD_SYS_PWROFF_GATING_REG);
		LOG("[1] 0x%x: 0x%x\n", VDD_SYS_PWROFF_GATING_REG, readl(VDD_SYS_PWROFF_GATING_REG));

		if (usb_en) {
			val = readl(CCU_CLK24M_GATE_EN_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_CLK24M_GATE_EN_REG, val);
			val |= USB24M_GATE_EN_MASK;
			writel(val, CCU_CLK24M_GATE_EN_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_CLK24M_GATE_EN_REG, readl(CCU_CLK24M_GATE_EN_REG));
		}

		if (usb0_en) {
			val = readl(CCU_USB_BUS_GATING_RST_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, val);
			val |= USBOHCI0_GATING_MASK;
			val |= USBEHCI0_GATING_MASK;
			writel(val, CCU_USB_BUS_GATING_RST_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, readl(CCU_USB_BUS_GATING_RST_REG));

			val = readl(CCU_USB0_CLOCK_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB0_CLOCK_REG, val);
			val |= SCLK_GATING_OHCI0_MASK;
			writel(val, CCU_USB0_CLOCK_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB0_CLOCK_REG, readl(CCU_USB0_CLOCK_REG));
		}

		if (usb1_en) {
			val = readl(CCU_USB_BUS_GATING_RST_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, val);
			val |= USBOHCI1_GATING_MASK;
			val |= USBEHCI1_GATING_MASK;
			writel(val, CCU_USB_BUS_GATING_RST_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB_BUS_GATING_RST_REG, readl(CCU_USB_BUS_GATING_RST_REG));

			val = readl(CCU_USB1_CLOCK_REG);
			LOG("[0] 0x%x: 0x%x\n", CCU_USB1_CLOCK_REG, val);
			val |= SCLK_GATING_OHCI1_MASK;
			writel(val, CCU_USB1_CLOCK_REG);
			LOG("[1] 0x%x: 0x%x\n", CCU_USB1_CLOCK_REG, readl(CCU_USB1_CLOCK_REG));
		}
	}
}

static void device_suspend(void)
{
	usb_standby_init();
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
	usb_standby_exit();
}

static void all_pll_set(enum pll_state sta)
{
	u32 reg_val, i;

	for (i = 0; i < (sizeof(pll_restore) / sizeof(pll_restore[0])); i++) {
		if (sta == pll_disable) {
			/* save PLLs SET */
			reg_val = readl(pll_restore[i].addr);
			pll_restore[i].factor = reg_val & PLL_FACKOR_MASK;

			/* disable PLLs */
			writel((readl(pll_restore[i].addr) & (~PLL_ENABLE_MASK)), pll_restore[i].addr);
			writel((readl(pll_restore[i].addr) & (~PLL_LDO_ENABLE_MASK)), pll_restore[i].addr);
			writel((readl(pll_restore[i].addr) & (~PLL_LOCK_ENABLE_MASK)), pll_restore[i].addr);
		} else if (sta == pll_enable) {
			reg_val = readl(pll_restore[i].addr);

			/* set n m p */
			reg_val &= ~PLL_FACKOR_MASK;
			reg_val |= pll_restore[i].factor;
			writel(reg_val, pll_restore[i].addr);

			/* set pll_en & pll_ldo_en, disable out_put_en */
			reg_val = readl(pll_restore[i].addr);
			reg_val |= 1 << PLL_ENABLE_SHIFT;
			reg_val |= 1 << PLL_LDO_ENABLE_SHIFT;
			reg_val &= ~PLL_OUTPUT_ENABLE_MASK;
			writel(reg_val, pll_restore[i].addr);

			/* set lock_en */
			reg_val = readl(pll_restore[i].addr);
			reg_val |= 1 << PLL_LOCK_ENABLE_SHIFT;
			writel(reg_val, pll_restore[i].addr);
			while (!(readl(pll_restore[i].addr) & PLL_LOCK_STATUS_MASK))
				;
		}
	}
	if (sta == pll_enable) {
		time_udelay(20);
		for (i = 0; i < (sizeof(pll_restore) / sizeof(pll_restore[0])); i++) {
			reg_val = readl(pll_restore[i].addr);
			reg_val |= 1 << PLL_OUTPUT_ENABLE_SHIFT;
			writel(reg_val, pll_restore[i].addr);
		}
	}
}

static void bus_clock_set(enum bus_clock_mode mode)
{
	u32 reg_val, bus_tick;

	for (bus_tick = 0; bus_tick < (sizeof(bus_restore) / sizeof(bus_restore[0])); bus_tick++) {
		reg_val = readl(bus_restore[bus_tick].busAddr);
		if ((mode == clock_bak) && (bus_restore[bus_tick].flag != 0)) {
			reg_val |=  bus_restore[bus_tick].M << CCU_FACTOR_M_SHIFT;
			writel(reg_val, bus_restore[bus_tick].busAddr);
			reg_val &= ~(CCU_CLK_SRC_SEL_MASK);
			reg_val |=  bus_restore[bus_tick].bus_clk_src << CCU_CLK_SRC_SEL_SHIFT;
			writel(reg_val, bus_restore[bus_tick].busAddr);
			bus_restore[bus_tick].flag = 0;
		} else {
			if (bus_restore[bus_tick].flag == 0) {
				bus_restore[bus_tick].bus_clk_src = (reg_val & CCU_CLK_SRC_SEL_MASK) >> CCU_CLK_SRC_SEL_SHIFT;
				bus_restore[bus_tick].M = (reg_val & CCU_FACTOR_M_MASK) >> CCU_FACTOR_M_SHIFT;
				bus_restore[bus_tick].flag = 1;
			}

			reg_val &= ~(CCU_CLK_SRC_SEL_MASK);
			reg_val |= (mode << CCU_CLK_SRC_SEL_SHIFT);
			writel(reg_val, bus_restore[bus_tick].busAddr);
			reg_val &= ~(CCU_FACTOR_M_MASK);
			writel(reg_val, bus_restore[bus_tick].busAddr);
		}
	}
}

static void clk_suspend(void)
{
	/* set ahb apb0 apb1 cpus(ahb) apbs0 clk to RC16M, clear factor m */
	bus_clock_set(rc16m);

	LOG("clk_suspend\n");
	/*
	 * set apbs1 clk to RC16M, clear factor m
	 * then change the baudrate of uart and twi...
	 */
	twi_clkchangecb(CCU_CLK_CLKCHG_REQ, iosc_freq);
	uart_clkchangecb(CCU_CLK_CLKCHG_REQ, iosc_freq);
	writel((readl(APBS1_CFG_REG) & (~APBS1_CLK_SRC_SEL_MASK)) | APBS1_CLK_SRC_SEL(2), APBS1_CFG_REG);
	writel((readl(APBS1_CFG_REG) & (~APBS1_FACTOR_M_MASK)), APBS1_CFG_REG);
	time_mdelay(10);
	uart_clkchangecb(CCU_CLK_CLKCHG_DONE, iosc_freq);
	twi_clkchangecb(CCU_CLK_CLKCHG_DONE, iosc_freq);
	time_mdelay(10);

	/* disable PLLs */
	all_pll_set(pll_disable);
	dcxo_disable();
}

static void clk_suspend_late(void)
{
	/* disable DSP audio pll */
	writel((readl(DSP_PLL_AUDIO_REG) & (~(PLL_ENABLE_MASK | PLL_LDO_ENABLE_MASK))), DSP_PLL_AUDIO_REG);
	writel((readl(DSP_PLL_AUDIO_REG) & (~PLL_LOCK_ENABLE_MASK)), DSP_PLL_AUDIO_REG);
}

static void clk_resume_early(void)
{
	dcxo_enable();
}

static void clk_resume(void)
{
	all_pll_set(pll_enable);

	/*
	 * set apbs1 clk to 24M
	 * then change the baudrate of uart and twi...
	 */
	twi_clkchangecb(CCU_CLK_CLKCHG_REQ, CCU_HOSC_FREQ);
	uart_clkchangecb(CCU_CLK_CLKCHG_REQ, CCU_HOSC_FREQ);
	writel((readl(APBS1_CFG_REG) & (~APBS1_FACTOR_M_MASK)) | APBS1_FACTOR_M(0), APBS1_CFG_REG);
	writel((readl(APBS1_CFG_REG) & (~APBS1_CLK_SRC_SEL_MASK)) | APBS1_CLK_SRC_SEL(0), APBS1_CFG_REG);
	time_mdelay(10);
	uart_clkchangecb(CCU_CLK_CLKCHG_DONE, CCU_HOSC_FREQ);
	twi_clkchangecb(CCU_CLK_CLKCHG_DONE, CCU_HOSC_FREQ);
	time_mdelay(10);

	/* restore ahb apb0 apb1 cpus(ahb) apbs0 clk */
	bus_clock_set(clock_bak);
}

/*standby power off cpu*/
void cpu_pll_off(void)
{
	int i;

	LOG("cpu off \n");
	/* backup cpua,cpub,dpu clk */
	cpu_clk_restore_bak[0] = readl(CPUA_CLK_REG);
	cpu_clk_restore_bak[1] = readl(CPUB_CLK_REG);
	cpu_clk_restore_bak[2] = readl(DCU_CLK_REG);

	/* set cpu clk rc16m */
	writel(((readl(CPUA_CLK_REG) & (~CPU_CLK_SRC_SEL_MASK)) | CPU_CLK_SRC_SEL(2)), CPUA_CLK_REG);
	writel((readl(CPUA_CLK_REG) & (~CPU_FACTOR_MASK)), CPUA_CLK_REG);

	writel(((readl(CPUB_CLK_REG) & (~CPU_CLK_SRC_SEL_MASK)) | CPU_CLK_SRC_SEL(2)), CPUB_CLK_REG);
	writel((readl(CPUB_CLK_REG) & (~CPU_FACTOR_MASK)), CPUB_CLK_REG);

	writel(((readl(DCU_CLK_REG) & (~DCU_CLK_SRC_SEL_MASK)) | DCU_CLK_SRC_SEL(2)), DCU_CLK_REG);
	writel((readl(DCU_CLK_REG) & (~DCU_FACTOR_P_MASK)), DCU_CLK_REG);

	/* close cpu1/2/3 pll */
	for (i = 1; i < 4; i++) {
		cpu_pll_restore_bak[i] = readl(CPU_PLL_REG(i));
		cpu_ssc_restore_bak[i] = readl(CPU_SSC_REG(i));
		writel((readl(CPU_PLL_REG(i)) & ~CPU_PLL_EN), CPU_PLL_REG(i));
		writel((readl(CPU_PLL_REG(i)) & ~CPU_PLL_LDO_EN), CPU_PLL_REG(i));
		writel((readl(CPU_PLL_REG(i)) & ~CPU_PLL_LOCK_EN), CPU_PLL_REG(i));
		writel((readl(CPU_PLL_REG(i)) | CPU_PLL_UPDATE), CPU_PLL_REG(i));
		writel((readl(CPU_SSC_REG(i)) | CPU_SSC_CLK_SEL), CPU_SSC_REG(i));

		while ((readl(CPU_PLL_REG(i)) & CPU_PLL_UPDATE))
			;
		writel((readl(CPU_SSC_REG(i)) & ~CPU_SSC_CLK_SEL), CPU_SSC_REG(i));
	}

#if 0
	/* close cpu0 pll */
	cpu_pll_restore_bak[0] = readl(CPU_PLL_REG(0));
	writel((readl(CPU_PLL_REG(0)) & ~CPU_PLL_EN), CPU_PLL_REG(0));
	writel((readl(CPU_PLL_REG(0)) & ~CPU_PLL_LDO_EN), CPU_PLL_REG(0));
	writel((readl(CPU_PLL_REG(0)) & ~CPU_PLL_LOCK_EN), CPU_PLL_REG(0));
#endif
}

/*standby power on cpu*/
void cpu_pll_on(void)
{
	int i;
	u32 reg_val;

	LOG("cpu on \n");
	/* set cpu1/2/3 pll */
	for (i = 1; i < 4; i++) {
		/* set pll on */
		writel((readl(CPU_SSC_REG(i)) | CPU_SSC_CLK_SEL), CPU_SSC_REG(i));

		writel((readl(CPU_PLL_REG(i)) | CPU_PLL_EN), CPU_PLL_REG(i));
		writel((readl(CPU_PLL_REG(i)) | CPU_PLL_LDO_EN), CPU_PLL_REG(i));
		writel((readl(CPU_PLL_REG(i)) | CPU_PLL_LOCK_EN), CPU_PLL_REG(i));
		writel((readl(CPU_PLL_REG(i)) | CPU_PLL_UPDATE), CPU_PLL_REG(i));
		while ((readl(CPU_PLL_REG(i)) & CPU_PLL_UPDATE))
			;

		while (!(readl(CPU_PLL_REG(i)) & CPU_PLL_LOCK_STATUS))
			;
	}

	time_mdelay(10);

	for (i = 1; i < 4; i++) {
		writel((readl(CPU_SSC_REG(i)) & ~CPU_SSC_CLK_SEL), CPU_SSC_REG(i));

		/* set ssc restore */
		reg_val = readl(CPU_SSC_REG(i));
		reg_val &= ~CPU_SSC_FACTOR;
		reg_val |= cpu_ssc_restore_bak[i] & CPU_SSC_FACTOR;
		writel(reg_val, CPU_SSC_REG(i));
		writel((readl(CPU_PLL_REG(i)) | CPU_PLL_UPDATE), CPU_PLL_REG(i));
		while ((readl(CPU_PLL_REG(i)) & CPU_PLL_UPDATE))
			;

		/* update factor */
		writel((readl(CPU_SSC_REG(i)) | CPU_SSC_EN), CPU_SSC_REG(i));
		writel((readl(CPU_PLL_REG(i)) | CPU_PLL_UPDATE), CPU_PLL_REG(i));
		while ((readl(CPU_PLL_REG(i)) & CPU_PLL_UPDATE))
			;

		reg_val = readl(CPU_PLL_REG(i));
		reg_val &= ~CPU_PLL_FACTOR;
		reg_val |= cpu_pll_restore_bak[i] & CPU_PLL_FACTOR;
		reg_val |= CPU_PLL_UPDATE;
		writel(reg_val, CPU_PLL_REG(i));
		while ((readl(CPU_PLL_REG(i)) & CPU_PLL_UPDATE))
			;

		writel((readl(CPU_SSC_REG(i)) & ~CPU_SSC_EN), CPU_SSC_REG(i));
		writel((readl(CPU_PLL_REG(i)) | CPU_PLL_UPDATE), CPU_PLL_REG(i));
		while ((readl(CPU_PLL_REG(i)) & CPU_PLL_UPDATE))
			;
	}

	time_udelay(20);

	/* set cpu0 pll */
#if 0
	reg_val = readl(CPU_PLL_REG(0));
	reg_val |= cpu_pll_restore_bak[0] & CPU_PLL_FACTOR;
	writel(reg_val, CPU_PLL_REG(0));
	writel((readl(CPU_PLL_REG(0)) | CPU_PLL_EN), CPU_PLL_REG(0));
	writel((readl(CPU_PLL_REG(0)) | CPU_PLL_LDO_EN), CPU_PLL_REG(0));
	writel((readl(CPU_PLL_REG(0)) & ~CPU_PLL_OUTPUT), CPU_PLL_REG(0));
	writel((readl(CPU_PLL_REG(0)) | CPU_PLL_LOCK_EN), CPU_PLL_REG(0));
	while (!(readl(CPU_PLL_REG(0)) & CPU_PLL_LOCK_STATUS))
			;
	time_udelay(20);
	writel((readl(CPU_PLL_REG(0)) | CPU_PLL_OUTPUT), CPU_PLL_REG(0));
#endif

	/* set cpua,cpub,dpu clk */
	writel((readl(CPUA_CLK_REG) | (cpu_clk_restore_bak[0] & CPU_FACTOR_MASK)), CPUA_CLK_REG);
	writel((readl(CPUA_CLK_REG) | (cpu_clk_restore_bak[0] & CPU_CLK_SRC_SEL_MASK)), CPUA_CLK_REG);

	writel((readl(CPUB_CLK_REG) | (cpu_clk_restore_bak[1] & CPU_FACTOR_MASK)), CPUB_CLK_REG);
	writel((readl(CPUB_CLK_REG) | (cpu_clk_restore_bak[1] & CPU_CLK_SRC_SEL_MASK)), CPUB_CLK_REG);

	writel((readl(DCU_CLK_REG) | (cpu_clk_restore_bak[2] & DCU_FACTOR_P_MASK)), DCU_CLK_REG);
	writel((readl(DCU_CLK_REG) | (cpu_clk_restore_bak[2] & DCU_CLK_SRC_SEL_MASK)), DCU_CLK_REG);
}

static void system_suspend(void)
{
	/* ANA */
	writel((readl(ANA_PWR_RST_REG) & (~ANA_EN_MASK)) | ANA_EN_CTRL(1), ANA_PWR_RST_REG);
	writel((readl(ANA_PWR_RST_REG) & (~ANA_EN_DSP_MASK)) | ANA_EN_DSP_CTRL(1), ANA_PWR_RST_REG);
	writel((readl(ANA_PWR_RST_REG) & (~ANA_EN_CPU_MASK)) | ANA_EN_CPU_CTRL(1), ANA_PWR_RST_REG);

	/* USB isolation */
	usb_iso_suspend();

	/* SYS reset */
	writel((readl(VDD_SYS_PWR_RST_REG) & (~VDD_SYS_MODULE_RST_MASK)) | VDD_SYS_MODULE_RST(0), VDD_SYS_PWR_RST_REG);

	/* SYS isolation */
	writel((readl(VDD_SYS_PWROFF_GATING_REG) & (~VDD_SYS2DSP_GATING_MASK)) | VDD_SYS2DSP_GATING(1), VDD_SYS_PWROFF_GATING_REG);
	writel((readl(VDD_SYS_PWROFF_GATING_REG) & (~VDD_SYS2CPUS_GATING_MASK)) | VDD_SYS2CPUS_GATING(1), VDD_SYS_PWROFF_GATING_REG);

	/* DSP isolation */
	writel((readl(DSP_SYS_PWR_RST_REG) & (~DSP_SYS_MODULE_RST_MASK)) | DSP_SYS_MODULE_RST(0), DSP_SYS_PWR_RST_REG);
	writel((readl(VDD_SYS_PWROFF_GATING_REG) & (~VDD_DSP2CPUS_GATING_MASK)) | VDD_DSP2CPUS_GATING(1), VDD_SYS_PWROFF_GATING_REG);
}

static void system_resume(void)
{
	/* DSP isolation */
	writel((readl(DSP_SYS_PWR_RST_REG) & (~DSP_SYS_MODULE_RST_MASK)) | DSP_SYS_MODULE_RST(1), DSP_SYS_PWR_RST_REG);
	writel((readl(VDD_SYS_PWROFF_GATING_REG) & (~VDD_DSP2CPUS_GATING_MASK)) | VDD_DSP2CPUS_GATING(0), VDD_SYS_PWROFF_GATING_REG);

	/* SYS isolation */
	writel((readl(VDD_SYS_PWROFF_GATING_REG) & (~VDD_SYS2DSP_GATING_MASK)) | VDD_SYS2DSP_GATING(0), VDD_SYS_PWROFF_GATING_REG);
	writel((readl(VDD_SYS_PWROFF_GATING_REG) & (~VDD_SYS2CPUS_GATING_MASK)) | VDD_SYS2CPUS_GATING(0), VDD_SYS_PWROFF_GATING_REG);

	/* SYS reset */
	writel((readl(VDD_SYS_PWR_RST_REG) & (~VDD_SYS_MODULE_RST_MASK)) | VDD_SYS_MODULE_RST(1), VDD_SYS_PWR_RST_REG);

	/* USB isolation */
	usb_iso_resume();

	/* ANA */
	writel((readl(ANA_PWR_RST_REG) & (~ANA_EN_MASK)) | ANA_EN_CTRL(0), ANA_PWR_RST_REG);
	writel((readl(ANA_PWR_RST_REG) & (~ANA_EN_DSP_MASK)) | ANA_EN_DSP_CTRL(0), ANA_PWR_RST_REG);
	writel((readl(ANA_PWR_RST_REG) & (~ANA_EN_CPU_MASK)) | ANA_EN_CPU_CTRL(0), ANA_PWR_RST_REG);
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

	suspend_para_prepare();

	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x1);

	cpucfg_cpu_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x2);

	device_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x3);

	cpu_pll_off();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x4);

	dram_suspend();
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

	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x22);
	system_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x3);

	clk_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x4);

	dram_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x5);

	cpu_pll_on();
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
#if 0
	iosc_freq_init();

	/* setcpus(ahb) apbs0 clk to RC16M, clear factor m */
	writel((readl(CPUS_CFG_REG) & (~CPUS_FACTOR_M_MASK)) | CPUS_CLK_SRC_SEL(2), CPUS_CFG_REG);
	writel((readl(CPUS_CFG_REG) & (~CPUS_FACTOR_M_MASK)), CPUS_CFG_REG);
	writel((readl(APBS0_CFG_REG) & (~APBS0_CLK_SRC_SEL_MASK)) | APBS0_CLK_SRC_SEL(2), APBS0_CFG_REG);
	writel((readl(APBS0_CFG_REG) & (~APBS0_FACTOR_M_MASK)), APBS0_CFG_REG);

	/*
	 * set apbs1 clk to RC16M, clear factor m
	 * then change the baudrate of uart and twi...
	 */
	twi_clkchangecb(CCU_CLK_CLKCHG_REQ, iosc_freq);
	uart_clkchangecb(CCU_CLK_CLKCHG_REQ, iosc_freq);
	writel((readl(APBS1_CFG_REG) & (~APBS1_CLK_SRC_SEL_MASK)) | APBS1_CLK_SRC_SEL(2), APBS1_CFG_REG);
	writel((readl(APBS1_CFG_REG) & (~APBS1_FACTOR_M_MASK)), APBS1_CFG_REG);
	uart_clkchangecb(CCU_CLK_CLKCHG_DONE, iosc_freq);
	twi_clkchangecb(CCU_CLK_CLKCHG_DONE, iosc_freq);
	time_mdelay(10);

	ccu_24mhosc_disable();
#endif
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
