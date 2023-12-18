// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Aaron <leafy.myeh@allwinnertech.com>
 *
 * MMC driver for allwinner sunxi platform.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */
#include <common.h>
#include <asm/io.h>
#include <asm/arch-sunxi/clock.h>
#include <asm/arch-sunxi/cpu.h>
#include <asm/arch-sunxi/mmc.h>
#include <asm/arch-sunxi/timer.h>
#include <malloc.h>
#include <mmc.h>
#include <sys_config.h>
#include <linux/libfdt.h>
#include <fdt_support.h>
#include <private_uboot.h>

#include "../sunxi_mmc.h"
#include "../mmc_def.h"
#include "sunxi_mmc_host_common.h"
#include "sunxi_mmc_host_tm1.h"

#if 1
#define  SMC_DATA_TIMEOUT     0xffffffU
#define  SMC_RESP_TIMEOUT     0xff
#else
#define  SMC_DATA_TIMEOUT     0x1ffffU
#define  SMC_RESP_TIMEOUT     0x2
#endif

/* For sun8iw6p1、sun8iw7p1 SDC2 */
#define SUNXI_DMA_TL_TM1_V4P10X        ((0x2<<28)|(7<<16)|8)

/* For other platform SDC0 */
#define SUNXI_DMA_TL_TM1_V4P1X        ((0x2<<28)|(7<<16)|248)

extern char *spd_name[];
extern struct sunxi_mmc_priv mmc_host[4];

static int mmc_init_default_timing_para(int sdc_no)
{
	struct sunxi_mmc_priv *mmcpriv = &mmc_host[sdc_no];

#if (!defined (CONFIG_MACH_SUN8IW7))
		mmcpriv->tm1.cur_spd_md = DS26_SDR12;
		mmcpriv->tm1.cur_freq = CLK_400K;
		mmcpriv->tm1.sample_point_cnt = MMC_CLK_SAMPLE_POINIT_MODE_1;

		mmcpriv->tm1.def_odly[DS26_SDR12*MAX_CLK_FREQ_NUM+CLK_400K] = TM1_OUT_PH180;
		mmcpriv->tm1.def_sdly[DS26_SDR12*MAX_CLK_FREQ_NUM+CLK_400K] = TM1_IN_PH180;
		mmcpriv->tm1.def_odly[DS26_SDR12*MAX_CLK_FREQ_NUM+CLK_25M] = TM1_OUT_PH180;
		mmcpriv->tm1.def_sdly[DS26_SDR12*MAX_CLK_FREQ_NUM+CLK_25M] = TM1_IN_PH180;

		mmcpriv->tm1.def_odly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_25M] = TM1_OUT_PH180;
		mmcpriv->tm1.def_sdly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_25M] = TM1_IN_PH90;
		mmcpriv->tm1.def_odly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_50M] = TM1_OUT_PH180;
		mmcpriv->tm1.def_sdly[HSSDR52_SDR25*MAX_CLK_FREQ_NUM+CLK_50M] = TM1_IN_PH90;

		mmcpriv->tm1.def_odly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_25M] = TM1_OUT_PH90;
		mmcpriv->tm1.def_sdly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_25M] = TM1_IN_PH180;
		mmcpriv->tm1.def_odly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_50M] = TM1_OUT_PH90;
		mmcpriv->tm1.def_sdly[HSDDR52_DDR50*MAX_CLK_FREQ_NUM+CLK_50M] = TM1_IN_PH180;

		mmcpriv->tm1.def_odly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_25M] = TM1_OUT_PH180;
		mmcpriv->tm1.def_sdly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_25M] = TM1_IN_PH90;
		mmcpriv->tm1.def_odly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_50M] = TM1_OUT_PH180;
		mmcpriv->tm1.def_sdly[HS200_SDR104*MAX_CLK_FREQ_NUM+CLK_50M] = TM1_IN_PH90;
#else
		mmcpriv->tm1.cur_spd_md = DS26_SDR12;
		mmcpriv->tm1.cur_freq = CLK_400K;
		mmcpriv->tm1.sample_point_cnt = MMC_CLK_SAMPLE_POINIT_MODE_1;

		mmcpriv->tm1.def_odly[DS26_SDR12 * MAX_CLK_FREQ_NUM + CLK_400K] = 0;
		mmcpriv->tm1.def_sdly[DS26_SDR12 * MAX_CLK_FREQ_NUM + CLK_400K] = 0;
		mmcpriv->tm1.def_odly[DS26_SDR12 * MAX_CLK_FREQ_NUM + CLK_25M] = 0;
		mmcpriv->tm1.def_sdly[DS26_SDR12 * MAX_CLK_FREQ_NUM + CLK_25M] = 0;

		mmcpriv->tm1.def_odly[HSSDR52_SDR25 * MAX_CLK_FREQ_NUM + CLK_25M] = 1;
		mmcpriv->tm1.def_sdly[HSSDR52_SDR25 * MAX_CLK_FREQ_NUM + CLK_25M] = 1;
		mmcpriv->tm1.def_odly[HSSDR52_SDR25 * MAX_CLK_FREQ_NUM + CLK_50M] = 1;
		mmcpriv->tm1.def_sdly[HSSDR52_SDR25 * MAX_CLK_FREQ_NUM + CLK_50M] = 1;

		mmcpriv->tm1.def_odly[HSDDR52_DDR50 * MAX_CLK_FREQ_NUM + CLK_25M] = 1;
		mmcpriv->tm1.def_sdly[HSDDR52_DDR50 * MAX_CLK_FREQ_NUM + CLK_25M] = 1;
		mmcpriv->tm1.def_odly[HSDDR52_DDR50 * MAX_CLK_FREQ_NUM + CLK_50M] = 1;
		mmcpriv->tm1.def_sdly[HSDDR52_DDR50 * MAX_CLK_FREQ_NUM + CLK_50M] = 1;
#endif

	return 0;
}

static void sunxi_mmc_clk_io_onoff(int sdc_no, int onoff, int reset_clk)
{
	int rval;
	struct sunxi_mmc_priv *priv = &mmc_host[sdc_no];

	/* config ahb clock */
#if (!defined (CONFIG_MACH_SUN8IW7))
	if (onoff) {
		rval = readl(priv->hclkrst);
		rval |= (1 << (16 + sdc_no));
		writel(rval, priv->hclkrst);
		rval = readl(priv->hclkbase);
		rval |= (1 << (0 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->mclkreg);
		rval |= (1U << 31);
		writel(rval, priv->mclkreg);
	} else {
		rval = readl(priv->mclkreg);
		rval &= ~(1U << 31);
		writel(rval, priv->mclkreg);

		rval = readl(priv->hclkbase);
		rval &= ~(1 << (0 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->hclkrst);
		rval &= ~(1 << (16 + sdc_no));
		writel(rval, priv->hclkrst);
	}
#elif defined(CONFIG_MACH_SUN60IW1)
	if (onoff) {
		rval = readl(priv->hclkrst);
		rval |= (1 << 16);
		writel(rval, priv->hclkrst);
		rval = readl(priv->hclkbase);
		rval |= (1 << 0);
		writel(rval, priv->hclkbase);

		rval = readl(priv->mclkreg);
		rval |= (1U << 31);
		writel(rval, priv->mclkreg);
	} else {
		rval = readl(priv->mclkreg);
		rval &= ~(1U << 31);
		writel(rval, priv->mclkreg);

		rval = readl(priv->hclkbase);
		rval &= ~(1 << 0);
		writel(rval, priv->hclkbase);

		rval = readl(priv->hclkrst);
		rval &= ~(1 << 16);
		writel(rval, priv->hclkrst);
	}
#else
	if (onoff) {
		rval = readl(priv->hclkrst);
		rval |= (1 << (8 + sdc_no));
		writel(rval, priv->hclkrst);
		rval = readl(priv->hclkbase);
		rval |= (1 << (8 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->mclkreg);
		rval |= (1U << 31);
		writel(rval, priv->mclkreg);
	} else {
		rval = readl(priv->mclkreg);
		rval &= ~(1U << 31);
		writel(rval, priv->mclkreg);

		rval = readl(priv->hclkbase);
		rval &= ~(1 << (8 + sdc_no));
		writel(rval, priv->hclkbase);

		rval = readl(priv->hclkrst);
		rval &= ~(1 << (8 + sdc_no));
		writel(rval, priv->hclkrst);
	}
#endif
	/* config mod clock */
	if (reset_clk) {
		rval = readl(priv->mclkreg);
		/*set to 24M default value*/
		rval &= ~(0x7fffffff);
		sunxi_r_op(priv, writel(rval, priv->mclkreg));
		priv->mod_clk = 24000000;
	}

}

static int mmc_config_delay(struct sunxi_mmc_priv *mmcpriv)
{
	unsigned int rval = 0;
	unsigned int spd_md, freq;
	u8 odly, sdly;
		spd_md = mmcpriv->tm1.cur_spd_md;
		freq = mmcpriv->tm1.cur_freq;

		if (mmcpriv->tm1.odly[spd_md*MAX_CLK_FREQ_NUM+freq] != 0xFF)
			odly = mmcpriv->tm1.odly[spd_md*MAX_CLK_FREQ_NUM+freq];
		else
			odly = mmcpriv->tm1.def_odly[spd_md*MAX_CLK_FREQ_NUM+freq];
		if (mmcpriv->tm1.sdly[spd_md*MAX_CLK_FREQ_NUM+freq] != 0xFF)
			sdly = mmcpriv->tm1.sdly[spd_md*MAX_CLK_FREQ_NUM+freq];
		else
			sdly = mmcpriv->tm1.def_sdly[spd_md*MAX_CLK_FREQ_NUM+freq];
		mmcpriv->tm1.cur_odly = odly;
		mmcpriv->tm1.cur_sdly = sdly;

		MMCDBG("%s: odly: %d   sldy: %d\n", __FUNCTION__, odly, sdly);
#if (!defined (CONFIG_MACH_SUN8IW7))
		rval = readl(&mmcpriv->reg->drv_dl);
		rval &= (~(0x3<<16));
		rval |= (((odly&0x1)<<16) | ((odly&0x1)<<17));
		sunxi_r_op(mmcpriv, writel(rval, &mmcpriv->reg->drv_dl));

		rval = readl(&mmcpriv->reg->ntsr);
		rval &= (~(0x3<<4));
		rval |= ((sdly&0x3)<<4);
		writel(rval, &mmcpriv->reg->ntsr);
#else
		rval = readl(&mmcpriv->reg->ntsr);
		rval &= (~((0x3 << 4) | (0x3 << 0)));
		rval |= (((odly & 0x3) << 0) | ((sdly & 0x3) << 4));
		writel(rval, &mmcpriv->reg->ntsr);
#endif

	return 0;
}

static int mmc_set_mod_clk(struct sunxi_mmc_priv *priv, unsigned int hz)
{
	unsigned int mod_hz, freq_id;
#ifdef FPGA_PLATFORM
	unsigned int rval;
#endif
	struct mmc *mmc = priv->mmc;
	u32 val = 0;
	mod_hz = 0;

	/*
	 * The MMC clock has an extra /2 post-divider when operating in the new
	 * mode.
	 */
	if (mmc->speed_mode == HSDDR52_DDR50)
		mod_hz = hz * 4;
	else
		mod_hz = hz * 2;

#ifndef CONFIG_MACH_SUN55IW3
	unsigned int pll, pll_hz, div, n;

	if (mod_hz <= 24000000) {
		pll = CCM_MMC_CTRL_OSCM24;
		pll_hz = 24000000;
	} else {
		pll = sunxi_mmc_get_src_clk_no(priv->mmc_no, mod_hz, 1);
		pll_hz = sunxi_host_src_clk(priv->mmc_no, (pll>>24), 1);
	}

	MMCDBG("pll config :%d : %d\n", pll, pll_hz);

	div = pll_hz / mod_hz;
	if (pll_hz % mod_hz)
		div++;

	n = 0;
	while (div > 16) {
		n++;
		div = (div + 1) / 2;
	}

	if (n > 3) {
		MMCINFO("mmc %u error cannot set clock to %u\n", priv->mmc_no,
		       hz);
		return -1;
	}
#else
#ifdef CONFIG_CLK_SUNXI
	int err = 0;
	u32 rate = 0;
	u32 rate_2 = 0;
	u32 retry_time = 0;
	struct clk *mclk = priv->cfg.clk_mmc;
	struct clk *sclk;
	struct clk *sclk_2;
	u32 src_clk = 0;

	/* hosc */
	sclk = clk_get(NULL, priv->cfg.pll0);
	if (IS_ERR_OR_NULL(sclk)) {
		MMCINFO("Error to get source clock %s\n", priv->cfg.pll0);
		return -1;
	}

	src_clk = clk_get_rate(sclk);
	if (mod_hz > src_clk) {
		clk_put(sclk);
		sclk = clk_get(NULL, priv->cfg.pll1);
	}
	if (IS_ERR_OR_NULL(sclk)) {
		MMCINFO("Error to get source clock %s\n", priv->cfg.pll1);
		return -1;
	}

clk_set_retry:
	err = clk_set_parent(mclk, sclk);
	if (err) {
		MMCINFO("set parent failed\n");
		clk_put(sclk);
		return -1;
	}

	rate = clk_round_rate(mclk, mod_hz);

	MMCDBG("get round rate %d\n", rate);

	if ((rate != mod_hz) && (mod_hz > src_clk) && (retry_time == 0)) {
		sclk_2 = clk_get(NULL, priv->cfg.pll2);
		if (IS_ERR_OR_NULL(sclk_2)) {
			MMCINFO("Error to get source clock pll_periph_another\n");
		} else {
			err = clk_set_parent(mclk, sclk_2);
			if (err) {
				MMCINFO("%s: set parent failed\n", __func__);
				clk_put(sclk_2);
				retry_time++;
				goto clk_set_retry;
			}

			rate_2 = clk_round_rate(mclk, mod_hz);

			MMCDBG("get round rate_2 = %d\n", rate_2);

			if (abs(mod_hz - rate_2) > abs(mod_hz - rate)) {
				MMCDBG("another SourceClk is worse\n");
				clk_put(sclk_2);
				retry_time++;
				goto clk_set_retry;
			} else {
				MMCDBG("another SourceClk is better and choose it\n");
				clk_put(sclk);
				sclk = sclk_2;
				rate = rate_2;
			}
		}
	}

	err = clk_disable(mclk);
	if (err) {
		MMCINFO("disable mmc clk err\n");
		return -1;
	}

	err = clk_set_rate(mclk, rate);
	if (err) {
		MMCINFO("set mclk rate error, rate %dHz\n",
			rate);
		clk_put(sclk);
		return -1;
	}

	err = clk_prepare_enable(mclk);
	if (err) {
		MMCINFO("enable mmc clk err\n");
		return -1;
	}

	src_clk = clk_get_rate(sclk);
	clk_put(sclk);

	MMCDBG("set round clock %d, soure clk is %d, mod_hz is %d\n", rate, src_clk, mod_hz);
#else
	MMCINFO("%s: need ccu config open, set clk = %d\n", __func__, mod_hz);
	return -1;
#endif
#endif

	freq_id = CLK_50M;
	/* determine delays */
	if (hz <= 400000) {
		freq_id = CLK_400K;
	} else if (hz <= 25000000) {
		freq_id = CLK_25M;
	} else if (hz <= 52000000) {
		freq_id = CLK_50M;
	} else if (hz <= 100000000)
		freq_id = CLK_100M;
	else if (hz <= 150000000)
		freq_id = CLK_150M;
	else if (hz <= 200000000)
		freq_id = CLK_200M;
	else {
		/* hz > 52000000 */
		freq_id = CLK_50M;
	}

	MMCDBG("freq_id:%d\n", freq_id);
#if (defined (CONFIG_MACH_SUN8IW7))
	val = 0x1 << 30;//CCM_MMC_CTRL_MODE_SEL_NEW;
#endif
#ifdef FPGA_PLATFORM
	if (readl(IOMEM_ADDR(SUNXI_MMMC_1X_2X_MODE_CTL_REG)) & (0x1 << 3)) {
		rval = readl(&priv->reg->ntsr);
		rval |= SUNXI_MMC_NTSR_MODE_SEL_NEW;
		writel(rval, &priv->reg->ntsr);
	} else {
		rval = readl(&priv->reg->ntsr);
		rval &= ~SUNXI_MMC_NTSR_MODE_SEL_NEW;
		writel(rval, &priv->reg->ntsr);
	}
#else
	setbits_le32(&priv->reg->ntsr, SUNXI_MMC_NTSR_MODE_SEL_NEW);
#endif

#ifdef FPGA_PLATFORM
	if (mod_hz > (400000 * 2)) {
		sunxi_r_op(priv, writel(CCM_MMC_CTRL_ENABLE,  priv->mclkreg));
	} else {
#ifndef CONFIG_MACH_SUN55IW3
		sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n) |
			CCM_MMC_CTRL_M(div) | val, priv->mclkreg));
#endif
	}
	if (hz <= 400000) {
		sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) & ~(0x1 << 7), &priv->reg->drv_dl));
	} else {
		sunxi_r_op(priv, writel(readl(&priv->reg->drv_dl) | (0x1 << 7), &priv->reg->drv_dl));
	}

#else
#ifndef CONFIG_MACH_SUN55IW3
	sunxi_r_op(priv, writel(pll | CCM_MMC_CTRL_N(n) |
	       CCM_MMC_CTRL_M(div) | val, priv->mclkreg));
#endif
#endif
	val = readl(&priv->reg->clkcr);
	val &= ~0xff;
	if (mmc->speed_mode == HSDDR52_DDR50)
		val |= 0x1;
	writel(val, &priv->reg->clkcr);

	priv->tm1.cur_spd_md = mmc->speed_mode;
	priv->tm1.cur_freq = freq_id;

	mmc_config_delay(priv);


	MMCDBG("mclk reg***%x\n", readl(priv->mclkreg));
	MMCDBG("clkcr reg***%x\n", readl(&priv->reg->clkcr));
#ifndef CONFIG_MACH_SUN55IW3
	MMCDBG("mmc %u set mod-clk req %u parent %u n %u m %u rate %u\n",
	      priv->mmc_no, mod_hz, pll_hz, 1u << n, div, pll_hz / (1u << n) / div);
#endif

	return 0;
}

static void mmc_ddr_mode_onoff(struct sunxi_mmc_priv *priv, int on)
{
	u32 rval = 0;

	rval = readl(&priv->reg->gctrl);
	rval &= (~(1U << 10));

	if (on) {
		rval |= (1U << 10);
		sunxi_r_op(priv, writel(rval, &priv->reg->gctrl));
		MMCDBG("set %d rgctrl 0x%x to enable ddr mode\n",
				priv->mmc_no, readl(&priv->reg->gctrl));
	} else {
		sunxi_r_op(priv, writel(rval, &priv->reg->gctrl));
		MMCDBG("set %d rgctrl 0x%x to disable ddr mode\n",
				priv->mmc_no, readl(&priv->reg->gctrl));
	}
}

static void sunxi_mmc_set_speed_mode(struct sunxi_mmc_priv *priv,
		struct mmc *mmc)
{
	/* set speed mode */
	if (mmc->speed_mode == HSDDR52_DDR50)
		mmc_ddr_mode_onoff(priv, 1);
	else
		mmc_ddr_mode_onoff(priv, 0);
}

static void sunxi_mmc_core_init(struct mmc *mmc)
{
	struct sunxi_mmc_priv *priv = mmc->priv;

	/* Reset controller */
	writel(SUNXI_MMC_GCTRL_RESET, &priv->reg->gctrl);
	udelay(1000);
	/* release eMMC reset signal */
	writel(1, &priv->reg->hwrst);
	writel(0, &priv->reg->hwrst);
	udelay(1000);
	writel(1, &priv->reg->hwrst);
	udelay(1000);
	/* Set Data & Response Timeout Value */
	writel((SMC_DATA_TIMEOUT<<8)|SMC_RESP_TIMEOUT, &priv->reg->timeout);
#if (!defined (CONFIG_MACH_SUN8IW7))
	writel((512<<16)|(1U<<2)|(1U<<0), &priv->reg->thldc);
#endif
	writel(0xdeb, &priv->reg->dbgc);
}

void sunxi_mmc_host_tm1_init(int sdc_no)
{
	struct sunxi_mmc_priv *mmcpriv = &mmc_host[sdc_no];
	struct mmc_config *cfg = &mmcpriv->cfg;

	cfg->f_max = 50000000; /* 50M */
#if (defined (CONFIG_MACH_SUN8IW7))
	mmcpriv->dma_tl = SUNXI_DMA_TL_TM1_V4P10X;
#else
	mmcpriv->dma_tl = SUNXI_DMA_TL_TM1_V4P1X;
#endif

	mmcpriv->sunxi_mmc_clk_io_onoff = sunxi_mmc_clk_io_onoff;
	mmcpriv->timing_mode = SUNXI_MMC_TIMING_MODE_1;
	mmcpriv->mmc_init_default_timing_para = mmc_init_default_timing_para;
	mmcpriv->mmc_set_mod_clk = mmc_set_mod_clk;
	mmcpriv->sunxi_mmc_set_speed_mode = sunxi_mmc_set_speed_mode;
	mmcpriv->sunxi_mmc_core_init = sunxi_mmc_core_init;
}
