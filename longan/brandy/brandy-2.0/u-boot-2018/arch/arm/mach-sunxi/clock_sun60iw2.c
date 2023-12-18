/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * SPDX-License-Identifier:	GPL-2.0+
*/

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include "private_uboot.h"

static void setbit(u32 cpux, u8 bit);
static void clearbit(u32 cpux, u8 bit);
void clock_init_uart(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* uart clock source is apb2 = 24M/M = 24M*/
	writel(APB2_CLK_SRC_OSC24M|
	       APB2_CLK_RATE_M(1),
	       &ccm->apb2_cfg);

	clrbits_le32(&ccm->uart_gate_reset,
		     1 << (uboot_spare_head.boot_data.uart_port));
	udelay(2);

	clrbits_le32(&ccm->uart_gate_reset,
		     1 << (RESET_SHIFT + uboot_spare_head.boot_data.uart_port));
	udelay(2);
	/* deassert uart reset */
	setbits_le32(&ccm->uart_gate_reset,
		     1 << (RESET_SHIFT + uboot_spare_head.boot_data.uart_port));
	/* open the clock for uart */
	setbits_le32(&ccm->uart_gate_reset,
		     1 << (uboot_spare_head.boot_data.uart_port));

}


static int clk_get_pll_para(struct core_pll_freq_tbl *factor, int pll_clk)
{
	int index;

	index = pll_clk / 24;
	factor->FactorP = 0;
	factor->FactorN = index;
	factor->FactorM = 0;

	return 0;
}

static void setbit(u32 cpux, u8 bit)
{
	u32 reg_val;
	reg_val = readl(cpux);
	reg_val |= (1 << bit);
	writel(reg_val, cpux);
}

static void clearbit(u32 cpux, u8 bit)
{
	u32 reg_val;
	reg_val = readl(cpux);
	reg_val &= ~(1 << bit);
	writel(reg_val, cpux);
}

static void enable_pll(u32 cpux, struct core_pll_freq_tbl *pll)
{
	u32 reg_val;
	/*pll gate disable*/
	clearbit(cpux, PLL_CPU1_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);

	reg_val = readl(cpux);
	reg_val &= ~((0x3 << 20) | (0x0f << 16) | (0xff << 8) | (0x0f << 0));
	/*set  PLL_CPU1 val: clk is frequency  ,PLL_OUTPUT= 24M*N/P/(M0*M1) */
	reg_val |= (0 << 20) | (pll->FactorP << 16) | (pll->FactorN << 8) |
		   (pll->FactorM << 0);
	writel(reg_val, cpux);
	__udelay(5);

	/*enable pll*/
	setbit(cpux, PLL_CPU1_CTRL_REG_PLL_EN_OFFSET);

	/*enable pll ldo*/
	setbit(cpux, PLL_CPU1_CTRL_REG_PLL_LDO_EN_OFFSET);
	__udelay(5);

	/*enable lock*/
	setbit(cpux, PLL_CPU1_CTRL_REG_LOCK_ENABLE_OFFSET);

	/* enable update bit */
	setbit(cpux, 26);
#ifndef FPGA_PLATFORM
	do {
		reg_val = readl(cpux);
	} while (!(reg_val & (0x1 << PLL_CPU1_CTRL_REG_LOCK_OFFSET)));
	__udelay(20);
#endif

	/* enable gate bit */
	setbit(cpux, PLL_CPU1_CTRL_REG_PLL_OUTPUT_GATE_OFFSET);

	/*disable lock*/
	clearbit(cpux, PLL_CPU1_CTRL_REG_LOCK_ENABLE_OFFSET);

	/* enable update bit */
	setbit(cpux, 26);
}

int clock_set_corepll(int frequency)
{
	unsigned int reg_val = 0;
	struct sunxi_cpu_pll_reg *const ccm =
		(struct sunxi_cpu_pll_reg *)CCMU_PLL_CPU0_CTRL_REG;
	struct core_pll_freq_tbl  pll_factor;

	if (frequency == clock_get_corepll())
		return 0;
	/* switch to 24M*/
	reg_val = 0x0305;
	writel(reg_val, &ccm->cpua_clk_reg);
	__udelay(20);

	reg_val = 0;
	writel(reg_val, &ccm->dsu_clk_reg);
	__udelay(20);

	/*get config para form freq table*/
	clk_get_pll_para(&pll_factor, frequency);
	enable_pll((u32)&ccm->pll_cpu1_ctrl_reg, &pll_factor);

	pll_factor.FactorP = 0;
	pll_factor.FactorM = 0;
	pll_factor.FactorN = 0x27; /*936M*/
	enable_pll((u32)&ccm->pll_cpu2_ctrl_reg, &pll_factor);

	/* switch core clk src to PLL_CPU1  PLL_CPU1/P*/
	reg_val = readl(&ccm->cpua_clk_reg);
	reg_val &= ~(0x03 << 24);
	reg_val |=  (0x03 << 24);
	reg_val &= ~(0x01 << 16); // P = 1
	reg_val |= (0x00 << 16);
	writel(reg_val, &ccm->cpua_clk_reg);
	__udelay(20);

	/* switch DSU clk src to PLL_CPU2  PLL_CPU2/P*/
	reg_val = readl(&ccm->dsu_clk_reg);
	reg_val &= ~(0x03 << 24);
	reg_val |=  (0x03 << 24);
	reg_val &= ~(0x01 << 16); // P = 1
	reg_val |= (0x00 << 16);
	writel(reg_val, &ccm->dsu_clk_reg);
	__udelay(20);

	return  0;
}

uint clock_get_pll6(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1, pll6;

	reg_val = readl(&ccm->pll6_cfg);

	factor_n = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 16) & 0x03) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll6 = (24 * factor_n /factor_p0/factor_m1)>>1;


	return pll6;
}

uint clock_get_ddrpll(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val, ddrpll;
	uint factor_n, factor_m1, factor_m0;
	/*fre: ddrpll = 24*N/M1/M0 */
	reg_val = readl(&ccm->pll_ddr_ctrl_reg);

	factor_n  = ((reg_val >> 8) & 0xff) + 1;
	factor_m0 = ((reg_val >> 1) & 0x01) + 1;
	factor_m1 = ((reg_val >> 0) & 0x01) + 1;
	ddrpll    = 24 * factor_n / factor_m0 / factor_m1;

	return ddrpll;
}

uint clock_get_peri1_pll6_pll4(int div)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_p0, factor_m1, pll6;

	reg_val = readl(&ccm->pll_peri1_ctrl_reg);

	/*pll = 24*N/M1/P0*/
	factor_n  = ((reg_val >> 8) & 0xff) + 1;
	factor_p0 = ((reg_val >> 16) & 0x07) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll6      = (24 * factor_n / factor_p0 / factor_m1) / div;

	return pll6;
}
uint clock_get_corepll(void)
{
	struct sunxi_cpu_pll_reg *const ccm =
		(struct sunxi_cpu_pll_reg *)CCMU_PLL_CPU0_CTRL_REG;
	unsigned int reg_val;
	int 	div_m, div_p, div_m1;
	int 	factor_n, factor_p;
	int 	clock, clock_src;

	reg_val   = readl(&ccm->cpua_clk_reg);
	clock_src = (reg_val >> 24) & 0x03;
	factor_p = 1 << ((reg_val >> 16) & 0x3);

	switch (clock_src) {
	case 0://OSC24M
		clock = 24;
		break;
	case 1://RTC32K
		clock = 32/1000 ;
		break;
	case 2://RC16M
		clock = 16;
		break;
	case 3://PLL_CPU1
		reg_val  = readl(&ccm->pll_cpu1_ctrl_reg);
		div_p    = ((reg_val >> 16) & 0xf) + 1;
		factor_n = ((reg_val >> 8) & 0xff);
		div_m    = ((reg_val >> 0) & 0xf) + 1;
		div_m1   = ((reg_val >> 20) & 0x3) + 1;

		clock = 24 * factor_n / div_p / (div_m * div_m1);
		break;
	default:
		return 0;
	}
	return clock / factor_p;
}


uint clock_get_axi(void)
{
	struct sunxi_cpu_pll_reg *const ccm =
		(struct sunxi_cpu_pll_reg *)CCMU_PLL_CPU0_CTRL_REG;
	unsigned int reg_val = 0;
	int factor = 0;
	int clock = 0;

	reg_val   = readl(&ccm->pll_cpu1_ctrl_reg);
	factor    = ((reg_val >> 0) & 0x03) + 1;
	clock = clock_get_corepll()/factor;

	return clock;
}


uint clock_get_ahb(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val = 0;
	int factor_m = 0, factor_n = 0;
	int clock = 0;
	int src = 0, src_clock = 0;

	reg_val = readl(&ccm->psi_ahb1_ahb2_cfg);
	src = (reg_val >> 24)&0x3;
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	factor_n  = 1;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//RC16M
		src_clock = 16;
		break;
	case 3://PLL_PERI0(1X)
		src_clock   = clock_get_pll6();
		break;
	default:
			return 0;
	}

	clock = src_clock/factor_m/factor_n;

	return clock;
}


uint clock_get_apb1(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val = 0;
	int src = 0, src_clock = 0;
	int clock = 0, factor_m = 0, factor_n = 0;

	reg_val = readl(&ccm->apb1_cfg);
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	factor_n  = 1;
	src = (reg_val >> 24)&0x3;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//PSI
		src_clock = 16;
		break;
	case 3://PLL_PERI0(1X)
		src_clock = clock_get_pll6();
		break;
	default:
		return 0;
	}

	clock = src_clock/factor_m/factor_n;

	return clock;
}

uint clock_get_apb2(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val = 0;
	int clock = 0, factor_m = 0, factor_n = 0;
	int src = 0, src_clock = 0;

	reg_val = readl(&ccm->apb2_cfg);
	src = (reg_val >> 24)&0x3;
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	factor_n  = 1;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//PSI
		src_clock = 16;
		break;
	case 3:	//PSI
		src_clock = clock_get_pll6();
		break;
	default:
			return 0;
	}

	clock = src_clock/factor_m/factor_n;

	return clock;

}


uint clock_get_mbus(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val;
	unsigned int src = 0, clock = 0, div = 0;
	reg_val = readl(&ccm->mbus_cfg);

	//get src
	src = (reg_val >> 24) & 0x3;
	//get div M, the divided clock is divided by M+1
	div = (reg_val & 0x3) + 1;

	switch (src) {
	case 0: //src is ddr
		clock = clock_get_ddrpll();
		break;
	case 1: //src is   pll_periph1(2x)/2
		clock = clock_get_peri1_pll6_pll4(2);
		break;
	case 2: //src is periph1_480
		clock = 480;
		break;
	case 3: //src is periph1_400
		clock = clock_get_peri1_pll6_pll4(3);
		break;
	}

	clock = clock / div;

	return clock;
}



int usb_open_clock(void)
{
	//disable otg clk gating and reset
	clrbits_le32(SUNXI_CCM_BASE + 0xA8C,
		     (0x01 << 5) | (0x01 << 8) | (0x01 << 21) | (0x1 << 24));
	udelay(50);

	//disable phy clk gating and reset
	clrbits_le32(SUNXI_CCM_BASE + 0xA70, (0x01 << 29)|(0x1 << 30));
	udelay(50);

	setbits_le32(SUNXI_CCM_BASE + 0xA8C, (0x01 << 24) | (0x01 << 21));
	udelay(50);

	//enable otg clk gating
	setbits_le32(SUNXI_CCM_BASE + 0xA8C, (0x01 << 8) | (0x01 << 5));
	udelay(50);

	__msdelay(1);

	return 0;
}

int usb_close_clock(void)
{
	/* AHB reset */
	clrbits_le32(SUNXI_CCM_BASE + 0xA8C, (0x01 << 24) | (0x01 << 21));
	__msdelay(1);

	clrbits_le32(SUNXI_CCM_BASE + 0xA8C, (0x01 << 8) | (0x01 << 5));
	__msdelay(1);

	clrbits_le32(SUNXI_CCM_BASE + 0xA70, (1 << 29) | (1 << 30));
	__msdelay(1);

	clrbits_le32(SUNXI_CCM_BASE + 0xA74, (1 << 29) | (1 << 30));
	__msdelay(1);

	return 0;
}


int sunxi_set_sramc_mode(void)
{
	u32 reg_val;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	/* SRAM Area C 128K Bytes Configuration by AHB ,default map to VE*/
	reg_val = readl(SUNXI_SRAMC_BASE);
	reg_val &= ~(0x7FFFFFFF);
	writel(reg_val, SUNXI_SRAMC_BASE);

	/* VE SRAM:set sram to normal mode, default boot mode */
	reg_val = readl(SUNXI_SRAMC_BASE + 0X0004);
	reg_val &= ~(0x1 << 24);
	writel(reg_val, SUNXI_SRAMC_BASE + 0X0004);

	/* VE gating&VE Bus Reset :brom set them, but not require now */
	reg_val = readl(&ccm->ve_clk_cfg);
	reg_val &= ~(0x1 << 0);
	reg_val &= ~(0x1 << 16);
	writel(reg_val, &ccm->ve_clk_cfg);

	/* DE gating&DE Bus Reset :brom set them, but not require now */
	reg_val = readl(&ccm->de_clk_cfg);
	reg_val &= ~(0x1 << 0);
	reg_val &= ~(0x1 << 16);
	writel(reg_val, &ccm->de_clk_cfg);
	return 0;
}

int sunxi_get_active_boot0_id(void)
{
	uint32_t val = *(uint32_t *)(SUNXI_RTC_BASE + 0x304);
	if (val & (1 << 15)) {
		return (val >> 8) & 0x7;
	} else {
		return (val >> 24) & 0x7;
	}
}

void clock_open_timer(int timernum)
{
	u32 reg_value = 0;

	reg_value = readl(SUNXI_CCM_BASE + 0x730 + timernum * 4);
	reg_value |= (0 <<24);
	reg_value |= (2 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + 0x730 + timernum * 4));

	/*enable timer*/
	reg_value = readl(SUNXI_CCM_BASE + 0x730 + timernum * 4);
	reg_value |= (1 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + 0x730 + timernum * 4));

	reg_value = readl(SUNXI_CCM_BASE + 0x74c);
	reg_value |= (1 << 16);
	writel(reg_value, (SUNXI_CCM_BASE + 0x74c));

	reg_value = readl(SUNXI_CCM_BASE + 0x74c);
	reg_value |= (1 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + 0x74c));
	__msdelay(1);

}

void clock_close_timer(int timernum)
{
	u32 reg_value = 0;

	/*disable timer*/
	reg_value = readl(SUNXI_CCM_BASE + 0x730 + timernum * 4);
	reg_value |= (0 << 31);
	writel(reg_value, (SUNXI_CCM_BASE + 0x730 + timernum * 4));

	reg_value = readl(SUNXI_CCM_BASE + 0x74c);
	reg_value |= (0 << 16);
	writel(reg_value, (SUNXI_CCM_BASE + 0x74c));

	reg_value = readl(SUNXI_CCM_BASE + 0x74c);
	reg_value |= (0 << 0);
	writel(reg_value, (SUNXI_CCM_BASE + 0x74c));
	__msdelay(1);
}

void clock_set_gic(void)
{
	u32 reg_value = 0;

	reg_value = readl(SUNXI_CCM_BASE + GIC_CLK_REG);
	reg_value |= ((1 << 31) | (3 << 24) | (0 << 0));
	writel(reg_value, SUNXI_CCM_BASE + GIC_CLK_REG);
	udelay(20);
}
