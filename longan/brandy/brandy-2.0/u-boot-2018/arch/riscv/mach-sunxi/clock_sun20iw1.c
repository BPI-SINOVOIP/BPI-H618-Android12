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
#include <asm/arch/prcm.h>
#include <asm/arch/efuse.h>
#include "private_uboot.h"

void clock_init_uart(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* uart clock source is apb2 */
	writel(APB2_CLK_SRC_OSC24M|
	       APB2_CLK_RATE_N_1|
	       APB2_CLK_RATE_M(1),
	       &ccm->apb2_cfg);

	/* open the clock for uart */
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

uint clock_get_pll_ddr(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint clock = 0;
	uint clock_src = 0;

	reg_val   = readl(&ccm->dram_clk_cfg);
	clock_src = (reg_val >> 24) & 0x03;

	switch (clock_src) {
	case 0:/*peri(2x)*/
		clock = clock_get_pll6() * 2;
		break;
	case 1:/*RTC32K*/
		clock = 800;
		break;
	case 2:/*pll_audio0*/
		clock = 0;
		break;
	case 3:
		clock = 0;
		break;
	default:
		return 0;
	}

	return clock;
}


uint clock_get_pll6(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	uint reg_val;
	uint factor_n, factor_m, factor_p0, pll6;

	reg_val = readl(&ccm->pll6_cfg);

	factor_p0 = ((reg_val >> 16) & 0x7) + 1;
	factor_n = ((reg_val >> 8) & 0xff) + 1;
	factor_m = ((reg_val >> 1) & 0x01) + 1;
	pll6 = (24 * factor_n / factor_m / factor_p0) >> 1;

	return pll6;
}

uint clock_get_corepll(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val;
	int 	factor_n;
	int 	clock, clock_src;

	reg_val   = readl(&ccm->riscv_clk_reg);
	clock_src = (reg_val >> 24) & 0x07;
	switch (clock_src) {
	case 0:/*OSC24M*/
		clock = 24;
		break;
	case 1:/*RTC32K*/
		clock = 32/1000 ;
		break;
	case 2:/*RC16M*/
		clock = 16;
		break;
	case 5:/*PLL_CPUX*/
		reg_val  = readl(&ccm->pll1_cfg);
		factor_n = ((reg_val >> 8) & 0xff) + 1;

		clock = 24 * factor_n;
		break;
	default:
		return 0;
	}
	return clock;
}


uint clock_get_axi(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned int reg_val = 0;
	int factor = 0;
	int clock = 0;

	reg_val   = readl(&ccm->cpu_axi_cfg);
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
	src = (reg_val >> 24) & 0x3;
	factor_m  = ((reg_val >> 0) & 0x03) + 1;
	factor_n  = (1 << ((reg_val >> 8) & 0x03));

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
		src_clock = clock_get_pll6();
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
	factor_n  = (1 << ((reg_val >> 8) & 0x03));
	src = (reg_val >> 24) & 0x3;

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
	src = (reg_val >> 24) & 0x3;
	factor_m  = ((reg_val >> 0) & 0x1f) + 1;
	factor_n  = (1 << ((reg_val >> 8) & 0x03));

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
	unsigned int clock;

	clock = clock_get_pll_ddr();
	clock = clock / 4;

	return clock;
}

static int clk_get_pll_para(struct core_pll_freq_tbl *factor, int pll_clk)
{
	int index;

	index = pll_clk / 24;
	factor->FactorP = 0;
	factor->FactorN = (index - 1);
	factor->FactorM = 0;

	return 0;
}

static u32 get_efuse_data(u32 offset, u32 mask, u32 shift)
{
	u32 reg_val = 0x0;
	u32 val = 0x0;

	reg_val = readl((SID_EFUSE + offset));
	val = (reg_val >> shift) & mask;

	return val;
}

static int get_cpu_clock_type(void)
{
	u32 chip_type = 0;
	u32 bin_type = 0;
	u32 cpu_clock = -1;

	chip_type = get_efuse_data(0x0, 0xffff, 0x0);

	if (chip_type == 0x7400) {
		cpu_clock = 1008;
	} else if (chip_type == 0x5c00) {
		bin_type = get_efuse_data(0x28, 0xf, 12);
		if (bin_type & 0x8) { //force
			cpu_clock = 720;
		} else if ((bin_type & 0x3) == 0x2) { //slow
			cpu_clock = 720;
		} else if ((bin_type == 0x1) || (bin_type == 0x0)) { //normal
			cpu_clock = 1008;
		} else {
			printf("can't support bin type %d\n", bin_type);
			return -1;
		}
	} else {
		printf("can't support chip type %d\n", chip_type);
		return -1;
	}

	return cpu_clock;
}

int clock_set_corepll(int frequency)
{
	unsigned int reg_val = 0;
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	struct core_pll_freq_tbl  pll_factor;
	u32 cpu_clock;

	cpu_clock = get_cpu_clock_type();
	if (cpu_clock < 0) {
		return -1;
	} else {
		if (frequency == clock_get_corepll())
			return 0;
		else if (frequency >= cpu_clock)
			frequency = cpu_clock;
	}

	/* switch to 24M*/
	reg_val   = readl(&ccm->riscv_clk_reg);
	reg_val &= ~(0x07 << 24);
	writel(reg_val, &ccm->riscv_clk_reg);
	__udelay(20);

	/*pll disable*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val &= ~(0x01 << 31);
	writel(reg_val, &ccm->pll1_cfg);

	/*pll output disable*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val &= ~(0x01 << 27);
	writel(reg_val, &ccm->pll1_cfg);

	/*get config para form freq table*/
	clk_get_pll_para(&pll_factor, frequency);

	reg_val = readl(&ccm->pll1_cfg);
	reg_val &= ~((0xff << 8)  | (0x03 << 0));
	reg_val |= (pll_factor.FactorN << 8) | (pll_factor.FactorM << 0) ;
	writel(reg_val, &ccm->pll1_cfg);
	__udelay(20);

	/*enable lock*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val |=  (0x1 << 29);
	writel(reg_val, &ccm->pll1_cfg);

	/*enable pll*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val |=	(0x1 << 31);
	writel(reg_val, &ccm->pll1_cfg);
#ifndef FPGA_PLATFORM
	do {
		reg_val = readl(&ccm->pll1_cfg);
	} while (!(reg_val & (0x1 << 28)));
#endif

	/*enable pll output*/
	reg_val = readl(&ccm->pll1_cfg);
	reg_val |=  (0x1 << 27);
	writel(reg_val, &ccm->pll1_cfg);
	__udelay(20);

	/* switch clk src to COREPLL*/
	reg_val = readl(&ccm->riscv_clk_reg);
	reg_val &= ~(0x07 << 24);
	reg_val |=  (0x05 << 24);
	writel(reg_val, &ccm->riscv_clk_reg);

	return  0;
}

int usb_open_clock(void)
{
	u32 reg_value = 0;
	volatile void __iomem *ccmu_base = IOMEM_ADDR(SUNXI_CCM_BASE);

	//USB0 Clock Reg
	//bit30: USB PHY0 reset
	//Bit29: Gating Special Clk for USB PHY0
	reg_value = readl(ccmu_base + 0xA70);
	reg_value |= (1 << 31);
	writel(reg_value, (ccmu_base + 0xA70));
	//delay some time
	__msdelay(1);

	reg_value = readl(ccmu_base + 0xA70);
	reg_value |= (1 << 30);
	writel(reg_value, (ccmu_base + 0xA70));
	//delay some time
	__msdelay(1);

	//USB BUS Gating Reset Reg
	//bit8:USB_OTG Gating
	reg_value = readl(ccmu_base + 0xA8C);
	reg_value |= (1 << 24);
	writel(reg_value, (ccmu_base + 0xA8C));

	//delay to wati SIE stable
	__msdelay(1);

	//USB BUS Gating Reset Reg: USB_OTG reset
	reg_value = readl(ccmu_base + 0xA8C);
	reg_value |= (1 << 8);
	writel(reg_value, (ccmu_base + 0xA8C));
	__msdelay(1);

	/* reg_value = readl(ccmu_base + 0x420);
	 * reg_value |= (0x01 << 0);
	 * writel(reg_value, (ccmu_base + 0x420));
	 * __msdelay(1); */

	return 0;
}

int usb_close_clock(void)
{
	u32 reg_value = 0;
	volatile void __iomem *ccmu_base = IOMEM_ADDR(SUNXI_CCM_BASE);

	/* AHB reset */
	reg_value = readl(ccmu_base + 0xA8C);
	reg_value &= ~(1 << 24);
	writel(reg_value, (ccmu_base + 0xA8C));
	__msdelay(1);

	reg_value = readl(ccmu_base + 0xA8C);
	reg_value &= ~(1 << 8);
	writel(reg_value, (ccmu_base + 0xA8C));
	__msdelay(1);

	/* reg_value = readl(ccmu_base + 0xcc);
	 * reg_value &= ~((1 << 29) | (1 << 30));
	 * writel(reg_value, (ccmu_base + 0xcc));
	 * __msdelay(1); */

	return 0;
}



