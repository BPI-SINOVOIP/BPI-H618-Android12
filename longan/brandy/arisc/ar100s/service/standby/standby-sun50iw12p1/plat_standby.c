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

#define WATCHDOG_KEYFIELD (0x16aa << 16)

extern u32 dram_crc_enable;
extern u32 dram_crc_src;
extern u32 dram_crc_len;
extern u32 volatile wakeup_source;
extern u32 axp_power_max;
extern u32 dtb_base;

static s32 result;
static u32 cpus_src;
static u32 suspend_lock;
static u32 pll_periph0, mbus;

static u32 vdd_cpu_grp;
static u32 vdd_cpu_num;
static u32 vdd_sys_grp;
static u32 vdd_sys_num;
static u32 vcc_pll_grp;
static u32 vcc_pll_num;
static u32 vcc_dram_grp;
static u32 vcc_dram_num;
static u32 power_key_grp;
static u32 power_key_num;

static u32 standby_type;

typedef struct BUS_PAMA {
	u32 busAddr;
	u8  bus_clk_src;
	u8  N;
	u8  M;
	u8  flag;//check the first write is or not default;
} rBUS_PAMA;
rBUS_PAMA bus_restore[8];

typedef struct PLL_PAMA {
	u32 addr;
	u8  pll_en;
	u8  lock_en;
	u8  n;
	u8  div2;//check the first write is or not default;
} rPLL_PAMA;
rPLL_PAMA pll_restore[12];

enum pll_state {
	pll_disable = 0,
	pll_enable  = 1
};

enum pmu_state {
	susper_standby = 0,
	normal_standby = 1,
	usb_standby = 2,
	elink_standby = 3
};

enum BUS_CLOCK_SOURCE {
	osc_24M = 0,
	rtc_32k = 1,
	rc16m  = 2,
	pll_peri0 = 3,
	dft_src = 4
};

enum start_code {
	COLD_START = 0x0,
	FAKE_POWER_OFF = 0x2,
	NORMAL_START = 0xf
};

static void bus_clock_source(enum BUS_CLOCK_SOURCE mode)
{
	u32 reg_val, bus_tick;

	bus_restore[0].busAddr = CCU_CPU_AXI_CFG_REG;
	bus_restore[1].busAddr = CCU_PSI_CFG_REG;
	bus_restore[2].busAddr = CCU_APB1_CFG_REG;
	bus_restore[3].busAddr = CCU_APB2_CFG_REG;
	bus_restore[4].busAddr = EDID_CFG_REG;
	bus_restore[5].busAddr = CPUS_CFG_REG;
	bus_restore[6].busAddr = APBS1_CFG_REG;
	bus_restore[7].busAddr = APBS2_CFG_REG;

	for (bus_tick = 0; bus_tick < (sizeof(bus_restore) / sizeof(bus_restore[0])); bus_tick++) {
		reg_val = readl(bus_restore[bus_tick].busAddr);
		if ((mode == dft_src) && (bus_restore[bus_tick].flag != 0)) {
			reg_val |=  bus_restore[bus_tick].N << 8;
			reg_val |=  bus_restore[bus_tick].M << 0;
			writel(reg_val, bus_restore[bus_tick].busAddr);
			reg_val &= ~(0x7 << 24);
			reg_val |=  bus_restore[bus_tick].bus_clk_src<<24;
			writel(reg_val, bus_restore[bus_tick].busAddr);
			bus_restore[bus_tick].flag = 0;
		} else {
			if (bus_restore[bus_tick].flag == 0) {
				bus_restore[bus_tick].bus_clk_src = (reg_val >> 24) & 0x7;
				bus_restore[bus_tick].N = (reg_val >> 8) & 0x3;
				bus_restore[bus_tick].M = reg_val & 0x3;
				bus_restore[bus_tick].flag = 1;
			} else
				bus_restore[bus_tick].flag = 1;
			reg_val &= ~(0x7 << 24);
			reg_val |= (mode << 24);
			writel(reg_val, bus_restore[bus_tick].busAddr);
			reg_val &= ~(0x3ff << 0);
			writel(reg_val, bus_restore[bus_tick].busAddr);
		}
		time_mdelay(10);
	}
}

static void all_pll_set(enum pll_state sta)
{
	u32 reg_val, i;

	pll_restore[0].addr = CCU_PLL_C0_REG;
	pll_restore[1].addr = CCU_PLL_DDR0_REG;
	pll_restore[2].addr = CCU_PLL_PERIPH0_REG;
	pll_restore[3].addr = CCU_PLL_PERIPH1_REG;
	pll_restore[4].addr = CCU_PLL_GPU0_CTRL_REG;
	pll_restore[5].addr = CCU_PLL_VIDEO0_CTRL_REG;
	pll_restore[6].addr = CCU_PLL_VIDEO1_CTRL_REG;
	pll_restore[7].addr = CCU_PLL_VIDEO2_CTRL_REG;
	pll_restore[8].addr = CCU_PLL_VE_CTRL_REG;
	pll_restore[9].addr = CCU_PLL_ADC_CTRL_REG;
	pll_restore[10].addr = CCU_PLL_VIDEO3_CTRL_REG;
	pll_restore[11].addr = CCU_PLL_AUDIO0_CTRL_REG;

	for (i = 0; i < (sizeof(pll_restore) / sizeof(pll_restore[0])); i++) {
		if (sta == pll_disable) {
			reg_val = readl(pll_restore[i].addr);
			pll_restore[i].pll_en = (reg_val >> 31) & 0x01;
			pll_restore[i].lock_en = (reg_val >> 29) & 0x01;
			pll_restore[i].n = (reg_val >> 8) & 0xff;
			pll_restore[i].div2 = (reg_val >> 0) & 0x3;

			reg_val &= (~(1 << 31));
			reg_val |= (sta << 31);
			reg_val |= (sta << 29);
			writel(reg_val, pll_restore[i].addr);
		} else if (sta == pll_enable) {
			reg_val = readl(pll_restore[i].addr);
			reg_val &= (~(1 << 31));
			reg_val |= (pll_restore[i].pll_en << 31);
			reg_val &= (~(1 << 29));
			reg_val |= (pll_restore[i].lock_en << 29);
			reg_val &= (~(0xff << 8));
			reg_val |= (pll_restore[i].n << 8);
			reg_val &= (~(0x3 << 0));
			reg_val |= (pll_restore[i].div2 << 0);
			writel(reg_val, pll_restore[i].addr);
		}
	}
}

static void rtc32k_disable(void)
{
	u32 reg_val;
	reg_val = readl(RTC_LOSC_CTRL);
	reg_val &= ~(0x1 << 0);
	reg_val |= (0x16aa << 16);
	writel(reg_val, RTC_LOSC_CTRL);
	writel(reg_val, RTC_LOSC_CTRL);
	time_udelay(1);
	reg_val = readl(RTC_LOSC_CTRL);
	reg_val &= ~(0x1 << 4);
	reg_val |= (0x16aa << 16);
	writel(reg_val, RTC_LOSC_CTRL);
	writel(reg_val, RTC_LOSC_CTRL);
	time_udelay(1);

	reg_val = readl(RTC_LOSC_OUT_GATING);
	reg_val &= ~(0x1 << 0);
	writel(reg_val, RTC_LOSC_OUT_GATING);
}

static void rtc32k_enable(void)
{
	u32 reg_val;
	reg_val = readl(RTC_LOSC_CTRL);
	reg_val |= (0x1 << 4);
	reg_val |= (0xa7 << 16);
	writel(reg_val, RTC_LOSC_CTRL);
	writel(reg_val, RTC_LOSC_CTRL);
	time_udelay(1);
	reg_val = readl(RTC_LOSC_CTRL);
	reg_val |= (0x1 << 0);
	reg_val |= (0x16aa << 16);
	writel(reg_val, RTC_LOSC_CTRL);
	writel(reg_val, RTC_LOSC_CTRL);
	time_udelay(1);
	reg_val = readl(RTC_LOSC_OUT_GATING);
	reg_val |= (0x1 << 0);
	writel(reg_val, RTC_LOSC_OUT_GATING);
}

static void pll_ldo_disable(void)
{
	u32 reg_val;
	reg_val = readl(PLL_CTRL_REG1);
	reg_val &= ~(0x1 << 0);
	reg_val |= (0xa7 << 24);
	writel(reg_val, PLL_CTRL_REG1);
	writel(reg_val, PLL_CTRL_REG1);
}

static void pll_ldo_enable(void)
{
	u32 reg_val;
	reg_val = readl(PLL_CTRL_REG1);
	reg_val |= (0x1 << 0);
	reg_val |= (0xa7 << 24);
	writel(reg_val, PLL_CTRL_REG1);
	writel(reg_val, PLL_CTRL_REG1);
}

void clk_enter_standby(void)
{
	bus_clock_source(rc16m);
	time_udelay(100);
	all_pll_set(pll_disable);
	time_udelay(100);
	rtc32k_disable();
	time_udelay(100);
	pll_ldo_disable();
	time_udelay(100);
}

void clk_exit_standby(void)
{
	pll_ldo_enable();
	time_udelay(100);
	rtc32k_enable();
	time_udelay(100);
	all_pll_set(pll_enable);
	time_udelay(100);
	bus_clock_source(dft_src);
	time_udelay(100);
}

void sys_powerdown(void)
{
	/* assert vdd-sys reset */
	writel(readl(VDD_SYS_PWR_RST_REG) & (~(0x1 << 0)), VDD_SYS_PWR_RST_REG);

	writel(readl(VDD_SYS_PWROFF_GATING_REG) | (0x1 << 2), VDD_SYS_PWROFF_GATING_REG);
	writel(readl(VDD_SYS_PWROFF_GATING_REG) | (0x1 << 8), VDD_SYS_PWROFF_GATING_REG);
}

void sys_powerup(void)
{
	writel(readl(VDD_SYS_PWROFF_GATING_REG)& (~(0x1 << 8)), VDD_SYS_PWROFF_GATING_REG);
	writel(readl(VDD_SYS_PWROFF_GATING_REG) & (~(0x1 << 2)), VDD_SYS_PWROFF_GATING_REG);

	/* deassert vdd-sys reset */
	writel(readl(VDD_SYS_PWR_RST_REG)| (0x1 << 0), VDD_SYS_PWR_RST_REG);
}

static s32 standby_dts_parse(void)
{
	void *fdt;
	int standby_node;
	int len;
	const char *pin_char;
	char pin_num_string[4];
	static u32 dts_has_parsed;

	if (!!dts_has_parsed)
		return 0;

	fdt = (void *)(dtb_base);

	standby_node = fdt_path_offset(fdt, "standby_param");
	if (standby_node < 0)
		return standby_node;

	/* parse vdd-cpu */
	pin_char = fdt_stringlist_get(fdt, standby_node, "vdd-cpu", 0, &len);
	if (strncmp(pin_char, "PL", strlen("PL")) == 0) {
		vdd_cpu_grp = PIN_GRP_PL;
		strncpy(pin_num_string, pin_char + strlen("PL"), 2);
		vdd_cpu_num = dstr2int(pin_num_string, 2);
	} else if (strncmp(pin_char, "PM", strlen("PM")) == 0) {
		vdd_cpu_grp = PIN_GRP_PM;
		strncpy(pin_num_string, pin_char + strlen("PM"), 2);
		vdd_cpu_num = dstr2int(pin_num_string, 2);
	}

	/* parse vdd-sys */
	pin_char = fdt_stringlist_get(fdt, standby_node, "vdd-sys", 0, &len);
	if (strncmp(pin_char, "PL", strlen("PL")) == 0) {
		vdd_sys_grp = PIN_GRP_PL;
		strncpy(pin_num_string, pin_char + strlen("PL"), 2);
		vdd_sys_num = dstr2int(pin_num_string, 2);
	} else if (strncmp(pin_char, "PM", strlen("PM")) == 0) {
		vdd_sys_grp = PIN_GRP_PM;
		strncpy(pin_num_string, pin_char + strlen("PM"), 2);
		vdd_sys_num = dstr2int(pin_num_string, 2);
	}

	/* parse vcc-pll */
	pin_char = fdt_stringlist_get(fdt, standby_node, "vcc-pll", 0, &len);
	if (strncmp(pin_char, "PL", strlen("PL")) == 0) {
		vcc_pll_grp = PIN_GRP_PL;
		strncpy(pin_num_string, pin_char + strlen("PL"), 2);
		vcc_pll_num = dstr2int(pin_num_string, 2);
	} else if (strncmp(pin_char, "PM", strlen("PM")) == 0) {
		vcc_pll_grp = PIN_GRP_PM;
		strncpy(pin_num_string, pin_char + strlen("PM"), 2);
		vcc_pll_num = dstr2int(pin_num_string, 2);
	}

	/* parse vcc-dram */
	pin_char = fdt_stringlist_get(fdt, standby_node, "vcc-dram", 0, &len);
	if (strncmp(pin_char, "PL", strlen("PL")) == 0) {
		vcc_dram_grp = PIN_GRP_PL;
		strncpy(pin_num_string, pin_char + strlen("PL"), 2);
		vcc_dram_num = dstr2int(pin_num_string, 2);
	} else if (strncmp(pin_char, "PM", strlen("PM")) == 0) {
		vcc_dram_grp = PIN_GRP_PM;
		strncpy(pin_num_string, pin_char + strlen("PM"), 2);
		vcc_dram_num = dstr2int(pin_num_string, 2);
	}

	/* parse power_key */
	pin_char = fdt_stringlist_get(fdt, standby_node, "power_key", 0, &len);
	if (strncmp(pin_char, "PL", strlen("PL")) == 0) {
		power_key_grp = PIN_GRP_PL;
		strncpy(pin_num_string, pin_char + strlen("PL"), 2);
		power_key_num = dstr2int(pin_num_string, 2);
	} else if (strncmp(pin_char, "PM", strlen("PM")) == 0) {
		power_key_grp = PIN_GRP_PM;
		strncpy(pin_num_string, pin_char + strlen("PM"), 2);
		power_key_num = dstr2int(pin_num_string, 2);
	}

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
	if (read_fake_poweroff_flag() == FAKE_POWER_OFF) {
		/* disable vdd-cpu */
		pin_set_multi_sel(vdd_cpu_grp, vdd_cpu_num, 0x1);
		writel(readl(PIN_DATA_REG(vdd_cpu_grp)) & (~(0x1 << vdd_cpu_num)), PIN_DATA_REG(vdd_cpu_grp));

		/* disable vdd-sys */
		pin_set_multi_sel(vdd_sys_grp, vdd_sys_num, 0x1);
		writel(readl(PIN_DATA_REG(vdd_sys_grp)) & (~(0x1 << vdd_sys_num)), PIN_DATA_REG(vdd_sys_grp));

		/* disable vcc-pll */
		pin_set_multi_sel(vcc_pll_grp, vcc_pll_num, 0x1);
		writel(readl(PIN_DATA_REG(vcc_pll_grp)) & (~(0x1 << vcc_pll_num)), PIN_DATA_REG(vcc_pll_grp));

		/* disable vcc-dram */
		pin_set_multi_sel(vcc_dram_grp, vcc_dram_num, 0x1);
		writel(readl(PIN_DATA_REG(vcc_dram_grp)) & (~(0x1 << vcc_dram_num)), PIN_DATA_REG(vcc_dram_grp));

		time_mdelay(10);
	}
}

static void dm_resume(void)
{
	if (read_fake_poweroff_flag() == FAKE_POWER_OFF) {
		/* enable vdd-cpu */
		pin_set_multi_sel(vdd_cpu_grp, vdd_cpu_num, 0x1);
		writel(readl(PIN_DATA_REG(vdd_cpu_grp)) | (0x1 << vdd_cpu_num), PIN_DATA_REG(vdd_cpu_grp));

		/* enable vdd-sys */
		pin_set_multi_sel(vdd_sys_grp, vdd_sys_num, 0x1);
		writel(readl(PIN_DATA_REG(vdd_sys_grp)) | (0x1 << vdd_sys_num), PIN_DATA_REG(vdd_sys_grp));

		/* enable vcc-pll */
		pin_set_multi_sel(vcc_pll_grp, vcc_pll_num, 0x1);
		writel(readl(PIN_DATA_REG(vcc_pll_grp)) | (0x1 << vcc_pll_num), PIN_DATA_REG(vcc_pll_grp));

		/* enable vcc-dram */
		pin_set_multi_sel(vcc_dram_grp, vcc_dram_num, 0x1);
		writel(readl(PIN_DATA_REG(vcc_dram_grp)) | (0x1 << vcc_dram_num), PIN_DATA_REG(vcc_dram_grp));

		time_mdelay(10);
	}

	/* set LDOB output to 1.8V */
	writel(readl(LDO_CTRL_REG) | (0x2f << 8), LDO_CTRL_REG);
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
}

static void dram_resume(void)
{
	/* restore dram controller and transing area. */
	LOG("power-up dram\n");

	/*
	 * mbus default clk src is 24MHz, switch to pll_periph0(x2),
	 * so before increase mbus freq, should set div firstly.
	 * by Superm Wu at 2015-09-18
	 */
#ifndef CFG_FPGA_PLATFORM
	writel(mbus&0x7, CCU_MBUS_CLK_REG);
	time_udelay(200);
	writel(mbus&((0x3 << 24) | 0x7), CCU_MBUS_CLK_REG);
	time_udelay(20);
	writel((readl(CCU_MBUS_CLK_REG) | (0x1 << 31)), CCU_MBUS_CLK_REG);
	time_udelay(20);
	writel((readl(CCU_MBUS_CLK_REG) | (0x1 << 30)), CCU_MBUS_CLK_REG);
	time_udelay(10000);
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

void svp_resume(void)
{
	u32 reg_val;

	/* open mbus clk*/
	reg_val = (0x01 << 31);
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(1);

	reg_val = readl(CCU_MBUS_CLK_REG);
	reg_val &= (~(1 << 30));
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(1);

	reg_val = readl(CCMU_DISP_BGR_REG);
	writel(reg_val | 0x00001, CCMU_DISP_BGR_REG);
	time_udelay(1);

	reg_val = readl(CCMU_DISP_BGR_REG);
	writel(reg_val | 0x10000, CCMU_DISP_BGR_REG);
	time_udelay(1);

	reg_val = readl(SVP_CLK_GATE_REG);
	writel(reg_val | 0x10, SVP_CLK_GATE_REG);
	time_udelay(1);

	reg_val = readl(SVP_RST_GATE_REG);
	writel(reg_val | 0x10, SVP_RST_GATE_REG);
	time_udelay(1);

	reg_val = readl(PNL_CLK_GATE_REG);
	writel(reg_val | 0x10, PNL_CLK_GATE_REG);
	time_udelay(1);

	reg_val = readl(CCU_MBUS_CLK_REG);
	reg_val |= (2 << 0);
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(1);
	reg_val = readl(CCU_MBUS_CLK_REG);
	reg_val |= (3 << 24);
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(1);

	/*reset mbus domain*/
	reg_val = readl(CCU_MBUS_CLK_REG);
	reg_val |= (1 << 30);
	writel(reg_val, CCU_MBUS_CLK_REG);
	time_udelay(1);
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
	writel(0, SMC_MST2_BYP_REG);
	writel(0xffffffff, SMC_MST0_SEC_REG);
	writel(0xffffffff, SMC_MST1_SEC_REG);
	writel(0xffffffff, SMC_MST2_SEC_REG);

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

static void device_suspend(void)
{
	smc_standby_init();
	pmu_standby_init();
	hwmsgbox_super_standby_init();
	ir_init();
}

static void device_resume(void)
{
	ir_exit();
	hwmsgbox_super_standby_exit();
	pmu_standby_exit();
	smc_standby_exit();
}


static void clk_suspend(void)
{
	clk_enter_standby();
}

static void clk_resume(void)
{
	clk_exit_standby();

	/* fix TV disp bug for TV303 version-A*/
	svp_resume();
}

static void system_suspend(void)
{
	if (!(standby_type & CPUS_WAKEUP_USB))
		sys_powerdown();
}

static void system_resume(void)
{
	if (!(standby_type & CPUS_WAKEUP_USB))
		sys_powerup();
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

	cpucfg_cpu_suspend_late();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x5);

	clk_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x6);

	system_suspend();
	save_state_flag(REC_ESTANDBY | REC_ENTER_INIT | 0x7);

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

	system_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x3);

	clk_resume();
	save_state_flag(REC_ESTANDBY | REC_ENTER_EXIT | 0x4);

	cpucfg_cpu_resume_early(resume_entry);
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
	cpus_src = readl(CPUS_CFG_REG) & CPUS_CLK_SRC_SEL_MASK;

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

	/* restore cpus source clock */
	writel((readl(CPUS_CFG_REG) & (~CPUS_CLK_SRC_SEL_MASK)) | cpus_src, CPUS_CFG_REG);

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
	if (entrypoint) {
		standby_entry(pmessage);
	} else {
		cpu_power_down(0, mpidr);
	}

	return 0;

}

static void system_shutdown(void)
{
	pmu_charging_reset();

	iosc_freq_init();

	/* set cpus clk to RC16M */
	writel((readl(CPUS_CFG_REG) & (~CPUS_CLK_SRC_SEL_MASK)) | CPUS_CLK_SRC_SEL(2), CPUS_CFG_REG);

	/*
	 * set apbs2 clk to RC16M,
	 * then change the baudrate of uart and twi...
	 */
	twi_clkchangecb(CCU_CLK_CLKCHG_REQ, iosc_freq);
	uart_clkchangecb(CCU_CLK_CLKCHG_REQ, iosc_freq);
	writel((readl(APBS2_CFG_REG) & (~APBS2_CLK_SRC_SEL_MASK)) | APBS2_CLK_SRC_SEL(2), APBS2_CFG_REG);
	uart_clkchangecb(CCU_CLK_CLKCHG_DONE, iosc_freq);
	twi_clkchangecb(CCU_CLK_CLKCHG_DONE, iosc_freq);
	time_mdelay(10);

	ccu_24mhosc_disable();

	pmu_shutdown();
}

static void watchdog_reset(void)
{
	LOG("watchdog reset\n");

	/* disable watchdog int */
	writel(0x0, R_WDOG_REG_BASE + 0x0);

	/* reset whole system */
	writel((0x1 | (0x1 << 8) | WATCHDOG_KEYFIELD), R_WDOG_REG_BASE + 0x10);

	/* set reset after 0.5s */
	writel(((0 << 4) | WATCHDOG_KEYFIELD), R_WDOG_REG_BASE + 0x14);
	mdelay(1);

	/* enable watchdog */
	writel((readl(R_WDOG_REG_BASE + 0x14) | 0x1 | WATCHDOG_KEYFIELD), R_WDOG_REG_BASE + 0x14);
	while (1)
		;
}


static void system_reset(void)
{
	watchdog_reset();
}

int sys_op(struct message *pmessage)
{
	u32 state = pmessage->paras[0];

	LOG("state:%x\n", state);

	switch (state) {
	case arisc_system_shutdown:
		{
			save_state_flag(REC_SHUTDOWN | 0x101);
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
	default:
		{
			WRN("invaid system power state (%d)\n", state);
			return -EINVAL;
		}
	}

	return 0;
}

s32 pwrkey_wakeup_init(void)
{
	u32 paras = GIC_R_GPIOL_NS_IRQ - 32;
	struct message message_ws;

	/* set pinctrl to external interrupt */
	pin_set_multi_sel(power_key_grp, power_key_num, 0xe);

	/* set nrgative as external interrupt trigger edge */
	writel(readl(PIN_INT_CFG_REG(power_key_grp, power_key_num)) & (~(0xf << PIN_INT_NUM_OFFSET(power_key_num))), PIN_INT_CFG_REG(power_key_grp, power_key_num));
	writel(readl(PIN_INT_CFG_REG(power_key_grp, power_key_num)) | (0x1 << PIN_INT_NUM_OFFSET(power_key_num)), PIN_INT_CFG_REG(power_key_grp, power_key_num));

	/* clean external interrupt mask */
	writel(readl(PIN_INT_STAT_REG(power_key_grp)) & (~(0x1 << power_key_num)), PIN_INT_STAT_REG(power_key_grp));
	writel(readl(PIN_INT_STAT_REG(power_key_grp)) | (0x1 << power_key_num), PIN_INT_STAT_REG(power_key_grp));

	/* enable external interrupt */
	writel(readl(PIN_INT_CTL_REG(power_key_grp)) & (~(0x1 << power_key_num)), PIN_INT_CTL_REG(power_key_grp));
	writel(readl(PIN_INT_CTL_REG(power_key_grp)) | (0x1 << power_key_num), PIN_INT_CTL_REG(power_key_grp));

	/* set wakeup source */
	message_ws.paras = &paras;
	set_wakeup_src(&message_ws);

	return 0;
}

void fake_pwroff_wakeup_src_init(void)
{
	pwrkey_wakeup_init();
	save_state_flag(REC_FAKEPOWEROFF | REC_BEFORE_INIT | 0x00);
}

s32 fake_poweroff(struct message *pmessage)
{
	save_state_flag(REC_FAKEPOWEROFF | REC_ENTER);
	standby_dts_parse();
	save_state_flag(REC_FAKEPOWEROFF | REC_ENTER | 0x01);

	fake_pwroff_wakeup_src_init();
	save_state_flag(REC_FAKEPOWEROFF | REC_ENTER | 0x02);

	standby_process_init(pmessage);
	save_state_flag(REC_FAKEPOWEROFF | REC_BEFORE_EXIT | 0x00);

	LOG("wait wakeup\n");

	wait_wakeup();
	save_state_flag(REC_FAKEPOWEROFF | REC_AFTER_EXIT | 0x00);

	dm_resume();
	save_state_flag(REC_FAKEPOWEROFF | REC_AFTER_EXIT | 0x01);

	save_fake_poweroff_flag(NORMAL_START);
	save_state_flag(REC_FAKEPOWEROFF | REC_AFTER_EXIT | 0x02);

	time_mdelay(5);
	save_state_flag(REC_FAKEPOWEROFF | REC_AFTER_EXIT | 0x03);

	watchdog_reset();
	save_state_flag(REC_FAKEPOWEROFF | REC_AFTER_EXIT | 0x04);

	return 0;
}
