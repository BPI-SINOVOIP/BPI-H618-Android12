/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
*/

#include <common.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include <asm/arch/clock.h>
#include <asm/arch/timer.h>
#include "private_uboot.h"

void clock_init_uart(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* uart clock source is apb2 */
	writel(APB2_CLK_SRC_OSC24M | APB2_CLK_RATE_N_1 | APB2_CLK_RATE_M(1),
	       &ccm->apb2_cfg);

	clrbits_le32(&ccm->uart_gate_reset +
			     uboot_spare_head.boot_data.uart_port * 4,
		     1 << GATING_SHIFT);
	udelay(2);

	clrbits_le32(&ccm->uart_gate_reset +
			     uboot_spare_head.boot_data.uart_port * 4,
		     1 << RESET_SHIFT);
	udelay(2);
	/* deassert uart reset */
	setbits_le32(&ccm->uart_gate_reset +
			     uboot_spare_head.boot_data.uart_port * 4,
		     1 << RESET_SHIFT);

	/* open the clock for uart */
	setbits_le32(&ccm->uart_gate_reset +
			     uboot_spare_head.boot_data.uart_port * 4,
		     1 << GATING_SHIFT);
}

// static int clk_get_pll_para(struct core_pll_freq_tbl *factor, int pll_clk)
// {
// 	int index;

// 	index = pll_clk / 24;
// 	factor->FactorP = 0;
// 	factor->FactorN = (index - 1);
// 	factor->FactorM = 0;

// 	return 0;
// }

int clock_set_corepll(int frequency)
{
	/*use PLL Contorl register = REF*/
	/*if current == frequency return */
	// unsigned int reg_val = 0;
	/* set N M P*/
	/* enable ldo_en disable pll_output_gate*/

	return  0;
}

uint clock_get_pll6(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_m0, factor_m1, pll6;

	reg_val = readl(&ccm->pll6_cfg);

	factor_n = ((reg_val >> 8) & 0xff) + 1;
	factor_m0 = ((reg_val >> 0) & 0x01) + 1;
	factor_m1 = ((reg_val >> 1) & 0x01) + 1;
	pll6 = (24 * factor_n /factor_m0/factor_m1)>>1;


	return pll6;
}

uint clock_get_corepll(void)
{
	/*use PLL Contorl register*/
	return 0;
}


uint clock_get_axi(void)
{

	return 0;
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
	factor_m  = ((reg_val >> 0) & 0x03) + 1;
	factor_n  = 1<< ((reg_val >> 8) & 0x03);

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
	factor_m  = ((reg_val >> 0) & 0x03) + 1;
	factor_n  = 1<<((reg_val >> 8) & 0x03);
	src = (reg_val >> 24)&0x3;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//PSI
		src_clock = clock_get_ahb();
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
	factor_m  = ((reg_val >> 0) & 0x03) + 1;
	factor_n  = ((reg_val >> 8) & 0x03) + 1;

	switch (src) {
	case 0://OSC24M
		src_clock = 24;
		break;
	case 1://CCMU_32K
		src_clock = 32/1000;
		break;
	case 2:	//PSI
		src_clock = clock_get_ahb();
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
	src = (reg_val >> 24)&0x3;
	//get div M, the divided clock is divided by M+1
	div = (reg_val&0x3) + 1;

	switch (src) {
	case 0://src is OSC24M
		clock = 24;
		break;
	case 1://src is   pll_periph0(1x)/2
		clock = clock_get_pll6()*2;
		break;
	case 2://src is pll_ddr0  --not set in boot
		clock   = 0;
		break;
	case 3://src is pll_ddr1 --not set in boot
		clock   = 0;
		break;
	}

	clock = clock/div;

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

	//USB0 Clock Reg
	//bit30: USB PHY0 reset
	//Bit29: Gating Special Clk for USB PHY0
	setbits_le32(SUNXI_CCM_BASE + 0xA70, (0x01 << 29));
	udelay(50);

	setbits_le32(SUNXI_CCM_BASE + 0xA70, (0x01 << 30));

	//USB1 Clock Reg
	//bit30: USB PHY1 reset
	//Bit29: Gating Special Clk for USB PHY1
	setbits_le32(SUNXI_CCM_BASE + 0xA74, (0x01 << 29));
	udelay(50);

	setbits_le32(SUNXI_CCM_BASE + 0xA74, (0x01 << 30));
	udelay(50);

	// clrbits_le32(SUNXI_EHCI1_BASE + 0x810, (0x01 << 3));

	// setbits_le32(SUNXI_USBOTG_BASE + 0x420, (0x01 << 0));

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
	uint32_t reg;
	/* reset */
	reg = readl(CCMU_TIMER0_BGR_REG + timernum * 0x70);
	reg &= ~(TIMER0_BGR_REG_TIMER0_RST_DE_ASSERT << TIMER0_BGR_REG_TIMER0_RST_OFFSET);
	writel(reg, CCMU_TIMER0_BGR_REG + timernum * 0x70);
	__msdelay(1);
	reg |= (TIMER0_BGR_REG_TIMER0_RST_DE_ASSERT << TIMER0_BGR_REG_TIMER0_RST_OFFSET);
	writel(reg, CCMU_TIMER0_BGR_REG + timernum * 0x70);
	/* gate */
	reg = readl(CCMU_TIMER0_BGR_REG + timernum * 0x70);
	reg &= ~(TIMER0_BGR_REG_TIMER0_GATING_PASS << TIMER0_BGR_REG_TIMER0_GATING_OFFSET);
	writel(reg, CCMU_TIMER0_BGR_REG + timernum * 0x70);
	__msdelay(1);
	reg |= (TIMER0_BGR_REG_TIMER0_GATING_PASS << TIMER0_BGR_REG_TIMER0_GATING_OFFSET);
	writel(reg, CCMU_TIMER0_BGR_REG + timernum * 0x70);
}

void clock_close_timer(int timernum)
{
	uint32_t reg;
	/* reset */
	reg = readl(CCMU_TIMER0_BGR_REG + timernum * 0x70);
	reg &= ~(TIMER0_BGR_REG_TIMER0_RST_DE_ASSERT << TIMER0_BGR_REG_TIMER0_RST_OFFSET);
	writel(reg, CCMU_TIMER0_BGR_REG + timernum * 0x70);
	__msdelay(1);
	reg |= (TIMER0_BGR_REG_TIMER0_RST_DE_ASSERT << TIMER0_BGR_REG_TIMER0_RST_OFFSET);
	writel(reg, CCMU_TIMER0_BGR_REG + timernum * 0x70);
	/* gate */
	reg = readl(CCMU_TIMER0_BGR_REG + timernum * 0x70);
	reg &= ~(TIMER0_BGR_REG_TIMER0_GATING_PASS << TIMER0_BGR_REG_TIMER0_GATING_OFFSET);
	writel(reg, CCMU_TIMER0_BGR_REG + timernum * 0x70);
}
