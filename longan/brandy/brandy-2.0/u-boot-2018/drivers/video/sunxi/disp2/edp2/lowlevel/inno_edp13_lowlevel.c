 /*
 * Copyright (c) 2007-2022 Allwinnertech Co., Ltd.
 * Author: huangyongxing <huangyongxing@allwinnertech.com>
 *
 * api for inno edp tx based on edp_1.3 hardware operation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <common.h>
#include <linux/io.h>
#include <linux/compat.h>
#include "inno_edp13_lowlevel.h"
#include "edp_lowlevel.h"
#include "../edp_core/edp_core.h"
#include "../edp_configs.h"
#include <asm-generic/gpio.h>
#include <sunxi_edp.h>

/*link symbol per TU*/
#define LS_PER_TU 64

static void __iomem *edp_base[2];
static u32 g_bpp;
static u32 g_lane_cnt;
static u32 training_interval_EQ;
static u32 training_interval_CR;

__attribute__((unused)) static spinlock_t aux_lock;

static struct recommand_corepll recom_corepll[] = {
	/* 1.62G*/
	{
		.prediv = 0x2,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x87,
		.postdiv = 0x1,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,
	},
	/*2.7G*/
	{
		.prediv = 0x2,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0xe1,
		.postdiv = 0x1,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,
	},
	{},
};

static struct recommand_pixpll recom_pixpll[] = {
	{
		.pixel_clk = 216000000,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0xc6,
		.plldiv_a = 0x1,
		.plldiv_b = 0x0,
		.plldiv_c = 0xb,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 54000000,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x63,
		.plldiv_a = 0x1,
		.plldiv_b = 0x1,
		.plldiv_c = 0xb,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 74250000,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x63,
		.plldiv_a = 0x1,
		.plldiv_b = 0x1,
		.plldiv_c = 0x8,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 148500000,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x63,
		.plldiv_a = 0x1,
		.plldiv_b = 0x1,
		.plldiv_c = 0x4,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 162000000,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x6c,
		.plldiv_a = 0x1,
		.plldiv_b = 0x1,
		.plldiv_c = 0x4,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 190320000,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x6c,
		.plldiv_a = 0x1,
		.plldiv_b = 0x1,
		.plldiv_c = 0x4,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 286272000,
		.prediv = 0x3,
		.fbdiv_h4 = 0x1,
		.fbdiv_l8 = 0x1e,
		.plldiv_a = 0x1,
		.plldiv_b = 0x0,
		.plldiv_c = 0x4,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 276528000,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x5c,
		.plldiv_a = 0x1,
		.plldiv_b = 0x0,
		.plldiv_c = 0x4,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 297000000,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x63,
		.plldiv_a = 0x1,
		.plldiv_b = 0x1,
		.plldiv_c = 0x2,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,

	},
	{
		.pixel_clk = 348577920,
		.prediv = 0x1,
		.fbdiv_h4 = 0x0,
		.fbdiv_l8 = 0x74,
		.plldiv_a = 0x1,
		.plldiv_b = 0x1,
		.plldiv_c = 0x2,
		.frac_pd = 0x3,
		.frac_h8 = 0x0,
		.frac_m8 = 0x0,
		.frac_l8 = 0x0,
	},
	{},
};


/*0:voltage_mode  1:cureent_mode*/
static void edp_mode_init(u32 sel, u32 mode)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_TX_PRESEL);
	if (mode) {
		reg_val = SET_BITS(24, 4, reg_val, 0xf);
	} else {
		reg_val = SET_BITS(24, 4, reg_val, 0x0);
	}
	writel(reg_val, edp_base[sel] + REG_EDP_TX_PRESEL);
}

void edp_aux_16m_config(u32 sel, u64 bit_rate)
{
	u32 bit_clock = 0;
	u32 div_16m = 0;
	u32 reg_val;

	if (bit_rate == BIT_RATE_1G62)
		bit_clock = 810;
	else if (bit_rate == BIT_RATE_2G7)
		bit_clock = 1350;
	else {
		EDP_ERR("no bit_clock match for bitrate:%lld\n", bit_rate);
		return;
	}

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_AUX_CLOCK);
	div_16m = (bit_clock / 8) / 8;
	reg_val = SET_BITS(0, 5, reg_val, div_16m);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_AUX_CLOCK);

	/*set main isel*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
	reg_val = SET_BITS(16, 5, reg_val, 0x1f);
	reg_val = SET_BITS(24, 5, reg_val, 0x1f);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);

	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	reg_val = SET_BITS(0, 5, reg_val, 0x1f);
	reg_val = SET_BITS(8, 5, reg_val, 0x1f);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);

	/*set pre isel*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_PRESEL);
	reg_val = SET_BITS(0, 15, reg_val, 0x0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_PRESEL);
}

void edp_corepll_config(u32 sel, u64 bit_rate)
{
	u32 reg_val;
	u32 index;

	if (bit_rate == BIT_RATE_1G62)
		index = 0;
	else
		index = 1;

	/*turnoff corepll*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll prediv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(8, 4, reg_val, recom_corepll[index].prediv);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll fbdiv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(16, 4, reg_val, recom_corepll[index].fbdiv_h4);
	reg_val = SET_BITS(24, 8, reg_val, recom_corepll[index].fbdiv_l8);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll postdiv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_POSDIV);
	reg_val = SET_BITS(2, 2, reg_val, recom_corepll[index].postdiv);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_POSDIV);

	/*config corepll frac_pd*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(4, 2, reg_val, recom_corepll[index].frac_pd);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	/*config corepll frac*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FRAC);
	reg_val = SET_BITS(0, 8, reg_val, recom_corepll[index].frac_h8);
	reg_val = SET_BITS(8, 8, reg_val, recom_corepll[index].frac_m8);
	reg_val = SET_BITS(16, 8, reg_val, recom_corepll[index].frac_l8);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FRAC);

	/*turnon corepll*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
}

void edp_pixpll_cfg(u32 sel, u32 pixel_clk)
{
	u32 reg_val;
	s32 i;

	for (i = 0; i < ARRAY_SIZE(recom_pixpll); i++) {
		if (recom_pixpll[i].pixel_clk == pixel_clk)
			break;
	}

	/*turnoff pixpll*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll prediv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(8, 6, reg_val, recom_pixpll[i].prediv);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll fbdiv*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(16, 4, reg_val, recom_pixpll[i].fbdiv_h4);
	reg_val = SET_BITS(24, 8, reg_val, recom_pixpll[i].fbdiv_l8);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll divabc*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_DIV);
	reg_val = SET_BITS(0, 5, reg_val, recom_pixpll[i].plldiv_a);
	reg_val = SET_BITS(8, 2, reg_val, recom_pixpll[i].plldiv_b);
	reg_val = SET_BITS(16, 5, reg_val, recom_pixpll[i].plldiv_c);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_DIV);

	/*config pixpll frac_pd*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(4, 2, reg_val, recom_pixpll[i].frac_pd);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);

	/*config pixpll frac*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FRAC);
	reg_val = SET_BITS(0, 8, reg_val, recom_pixpll[i].frac_h8);
	reg_val = SET_BITS(8, 8, reg_val, recom_pixpll[i].frac_m8);
	reg_val = SET_BITS(16, 8, reg_val, recom_pixpll[i].frac_l8);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FRAC);

	/*turnon pixpll*/
	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	reg_val = SET_BITS(0, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
}

void edp_set_misc(u32 sel, u32 misc0_val, u32 misc1_val)
{
	u32 reg_val;

	/*misc0 setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_MSA_MISC0);
	reg_val = SET_BITS(24, 8, reg_val, misc0_val);
	writel(reg_val, edp_base[sel] + REG_EDP_MSA_MISC0);

	/*misc1 setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_MSA_MISC1);
	reg_val = SET_BITS(24, 8, reg_val, misc1_val);
	writel(reg_val, edp_base[sel] + REG_EDP_MSA_MISC1);
}

void edp_lane_config(u32 sel, u32 lane_cnt, u64 bit_rate)
{
	u32 reg_val;

	if ((lane_cnt < 0) || (lane_cnt > 4)) {
		EDP_WRN("unsupport lane number!\n");
	}

	g_lane_cnt = lane_cnt;

	/*config lane number*/
	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	switch (lane_cnt) {
	case 0:
	case 3:
		EDP_WRN("edp lane number can not be configed to 0/3!\n");
	case 1:
		reg_val = SET_BITS(6, 2, reg_val, 0x0);
		reg_val = SET_BITS(8, 4, reg_val, 0x01);
		break;
	case 2:
		reg_val = SET_BITS(6, 2, reg_val, 0x1);
		reg_val = SET_BITS(8, 4, reg_val, 0x3);
		break;
	case 4:
		reg_val = SET_BITS(6, 2, reg_val, 0x2);
		reg_val = SET_BITS(8, 4, reg_val, 0xf);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);

	/*config lane bit rate*/
	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	switch (bit_rate) {
	case BIT_RATE_1G62:
		reg_val = SET_BITS(26, 3, reg_val, 0x0);
		reg_val = SET_BITS(4, 2, reg_val, 0x0);
		break;
	case BIT_RATE_2G7:
	default:
		reg_val = SET_BITS(26, 3, reg_val, 0x0);
		reg_val = SET_BITS(4, 2, reg_val, 0x1);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);
}

void edp_assr_enable(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(24, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
}

void edp_assr_disable(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(24, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
}

void edp_video_stream_enable(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(5, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
}

void edp_video_stream_disable(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(5, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
}

void edp_hal_set_training_pattern(u32 sel, u32 pattern)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	reg_val = SET_BITS(0, 4, reg_val, pattern);
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);
}

void edp_link_lane_para_setting(u32 sel, u8 sw0, u8 pre0, u8 sw1, u8 pre1, u8 sw2, u8 pre2,
		 u8 sw3, u8 pre3)
{
	u32 reg_val;

	/*sw0*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
	if (sw0 == 0)
		reg_val = SET_BITS(0, 4, reg_val, 0);
	else if (sw0 == 1)
		reg_val = SET_BITS(0, 4, reg_val, 2);
	else if (sw0 == 2)
		reg_val = SET_BITS(0, 4, reg_val, 4);
	else if (sw0 == 3)
		reg_val = SET_BITS(0, 4, reg_val, 6);
	else
		reg_val = SET_BITS(0, 4, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);

	/*sw1*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
	if (sw1 == 0)
		reg_val = SET_BITS(4, 4, reg_val, 0);
	else if (sw1 == 1)
		reg_val = SET_BITS(4, 4, reg_val, 2);
	else if (sw1 == 2)
		reg_val = SET_BITS(4, 4, reg_val, 4);
	else if (sw1 == 3)
		reg_val = SET_BITS(4, 4, reg_val, 6);
	else
		reg_val = SET_BITS(4, 4, reg_val, 0);

	writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);

	/*sw2*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
	if (sw2 == 0)
		reg_val = SET_BITS(24, 4, reg_val, 0);
	else if (sw2 == 1)
		reg_val = SET_BITS(24, 4, reg_val, 2);
	else if (sw2 == 2)
		reg_val = SET_BITS(24, 4, reg_val, 4);
	else if (sw2 == 3)
		reg_val = SET_BITS(24, 4, reg_val, 6);
	else
		reg_val = SET_BITS(24, 4, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX32_ISEL_DRV);

	/*sw3*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
	if (sw3 == 0)
		reg_val = SET_BITS(28, 4, reg_val, 0);
	else if (sw3 == 1)
		reg_val = SET_BITS(28, 4, reg_val, 2);
	else if (sw3 == 2)
		reg_val = SET_BITS(28, 4, reg_val, 4);
	else if (sw3 == 3)
		reg_val = SET_BITS(28, 4, reg_val, 6);
	else
		reg_val = SET_BITS(28, 4, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX32_ISEL_DRV);

	/*pre0*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	if (pre0 == 0)
		reg_val = SET_BITS(24, 4, reg_val, 0);
	else if (pre0 == 1)
		reg_val = SET_BITS(24, 4, reg_val, 1);
	else if (pre0 == 2)
		reg_val = SET_BITS(24, 4, reg_val, 2);
	else if (pre0 == 3)
		reg_val = SET_BITS(24, 4, reg_val, 3);
	else
		reg_val = SET_BITS(24, 4, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);

	/*pre1*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	if (pre1 == 0)
		reg_val = SET_BITS(28, 4, reg_val, 0);
	else if (pre0 == 1)
		reg_val = SET_BITS(28, 4, reg_val, 1);
	else if (pre0 == 2)
		reg_val = SET_BITS(28, 4, reg_val, 2);
	else if (pre0 == 3)
		reg_val = SET_BITS(28, 4, reg_val, 3);
	else
		reg_val = SET_BITS(28, 4, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);

	/*pre2*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	if (pre2 == 0)
		reg_val = SET_BITS(16, 4, reg_val, 0);
	else if (pre2 == 1)
		reg_val = SET_BITS(16, 4, reg_val, 1);
	else if (pre2 == 2)
		reg_val = SET_BITS(16, 4, reg_val, 2);
	else if (pre2 == 3)
		reg_val = SET_BITS(16, 4, reg_val, 3);
	else
		reg_val = SET_BITS(16, 4, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);

	/*pre3*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	if (pre3 == 0)
		reg_val = SET_BITS(20, 4, reg_val, 0);
	else if (pre3 == 1)
		reg_val = SET_BITS(20, 4, reg_val, 1);
	else if (pre3 == 2)
		reg_val = SET_BITS(20, 4, reg_val, 2);
	else if (pre3 == 3)
		reg_val = SET_BITS(20, 4, reg_val, 3);
	else
		reg_val = SET_BITS(20, 4, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);

	/*set main isel*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
	reg_val = SET_BITS(16, 5, reg_val, 0x1f);
	reg_val = SET_BITS(24, 5, reg_val, 0x1f);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);

	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	reg_val = SET_BITS(0, 5, reg_val, 0x1f);
	reg_val = SET_BITS(8, 5, reg_val, 0x1f);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);

	/*set pre isel*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_PRESEL);
	reg_val = SET_BITS(0, 15, reg_val, 0x0);
	writel(reg_val, edp_base[sel] + REG_EDP_TX_PRESEL);
}

void edp_audio_stream_vblank_setting(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
	reg_val = SET_BITS(1, 1, reg_val, enable);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
}

void edp_audio_timestamp_vblank_setting(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
	reg_val = SET_BITS(0, 1, reg_val, enable);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
}

void edp_audio_stream_hblank_setting(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
	reg_val = SET_BITS(1, 1, reg_val, enable);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
}

void edp_audio_timestamp_hblank_setting(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
	reg_val = SET_BITS(0, 1, reg_val, enable);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
}

void edp_audio_interface_config(u32 sel, u32 interface)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = SET_BITS(0, 1, reg_val, interface);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO);
}

void edp_audio_channel_config(u32 sel, u32 chn_num)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);

	switch (chn_num) {
	case 2:
		reg_val = SET_BITS(12, 3, reg_val, 0x1);
		reg_val = SET_BITS(1, 2, reg_val, 0x3);
		break;
	case 8:
		reg_val = SET_BITS(12, 3, reg_val, 0x2);
		reg_val = SET_BITS(1, 4, reg_val, 0xf);
		break;
	case 1:
	default:
		reg_val = SET_BITS(12, 3, reg_val, 0x0);
		reg_val = SET_BITS(1, 1, reg_val, 0x1);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO);
}

void edp_audio_mute_config(u32 sel, bool mute)
{

	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = SET_BITS(15, 1, reg_val, mute);
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO);
}

void edp_audio_data_width_config(u32 sel, u32 data_width)
{

	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	switch (data_width) {
	case 20:
		reg_val = SET_BITS(5, 5, reg_val, 0x18);
		break;
	case 24:
		reg_val = SET_BITS(5, 5, reg_val, 0x14);
		break;
	case 16:
	default:
		reg_val = SET_BITS(5, 5, reg_val, 0x10);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_AUDIO);
}

void edp_audio_soft_reset(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_RESET);
	reg_val = SET_BITS(3, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_RESET);
	udelay(5);
	reg_val = SET_BITS(3, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_RESET);
}

void edp_set_input_video_mapping(u32 sel, enum edp_video_mapping_e mapping)
{
	u32 reg_val;
	u32 mapping_val = 0;
	u32 misc0_val = 0;
	u32 misc1_val = 0;

	switch (mapping) {
	case RGB_6BIT:
		mapping_val = 0;
		misc0_val = (0 << 5);
		g_bpp = 18;
		break;
	case RGB_8BIT:
		mapping_val = 1;
		misc0_val = (1 << 5);
		g_bpp = 24;
		break;
	case RGB_10BIT:
		mapping_val = 2;
		misc0_val = (2 << 5);
		g_bpp = 30;
		break;
	case RGB_12BIT:
		mapping_val = 3;
		misc0_val = (3 << 5);
		g_bpp = 36;
		break;
	case RGB_16BIT:
		mapping_val = 4;
		misc0_val = (4 << 5);
		g_bpp = 48;
		break;
	case YCBCR444_8BIT:
		mapping_val = 5;
		misc0_val = (1 << 5) | (1 << 2);
		g_bpp = 24;
		break;
	case YCBCR444_10BIT:
		mapping_val = 6;
		misc0_val = (2 << 5) | (1 << 2);
		g_bpp = 30;
		break;
	case YCBCR444_12BIT:
		mapping_val = 7;
		misc0_val = (3 << 5) | (1 << 2);
		g_bpp = 36;
		break;
	case YCBCR444_16BIT:
		mapping_val = 8;
		misc0_val = (4 << 5) | (1 << 2);
		g_bpp = 48;
		break;
	case YCBCR422_8BIT:
		mapping_val = 9;
		misc0_val = (1 << 5) | (1 << 1);
		g_bpp = 16;
		break;
	case YCBCR422_10BIT:
		mapping_val = 10;
		misc0_val = (2 << 5) | (1 << 1);
		g_bpp = 20;
		break;
	case YCBCR422_12BIT:
		mapping_val = 11;
		misc0_val = (3 << 5) | (1 << 1);
		g_bpp = 24;
		break;
	case YCBCR422_16BIT:
		mapping_val = 12;
		misc0_val = (4 << 5) | (1 << 1);
		g_bpp = 32;
		break;
	}

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);
	reg_val = SET_BITS(16, 5, reg_val, mapping_val);
	writel(reg_val, edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);

	edp_set_misc(sel, misc0_val, misc1_val);
}

s32 edp_bist_test(u32 sel, struct edp_tx_core *edp_core)
{
	u32 reg_val;
	s32 ret;

	/*set bist_test_sel to 1*/
	reg_val = readl(edp_base[sel] + REG_EDP_BIST_CFG);
	reg_val = SET_BITS(0, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_BIST_CFG);

	/*assert reset pin*/
	if (edp_core->rst_gpio) {
		gpio_direction_output(edp_core->rst_gpio, 0);
	}

	/*set bist_test_en to 1*/
	reg_val = readl(edp_base[sel] + REG_EDP_BIST_CFG);
	reg_val = SET_BITS(1, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_BIST_CFG);

	udelay(5);

	/*wait for bist_test_done to 1*/
	while (1) {
		reg_val = readl(edp_base[sel] + REG_EDP_BIST_CFG);
		reg_val = GET_BITS(4, 1, reg_val);
		if (reg_val == 1)
			break;
	}

	/*checke bist_test_done*/
	reg_val = readl(edp_base[sel] + REG_EDP_BIST_CFG);
	reg_val = GET_BITS(5, 1, reg_val);
	if (reg_val == 1)
		ret = RET_OK;
	else
		ret = RET_FAIL;

	/*deassert reset pin and disable bist_test_sel to exit bist*/
	if (edp_core->rst_gpio) {
		gpio_direction_output(edp_core->rst_gpio, 1);
	}
	writel(0x0, edp_base[sel] + REG_EDP_BIST_CFG);

	return ret;
}

void edp_phy_soft_reset(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_RESET);
	reg_val = SET_BITS(1, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_RESET);
	udelay(5);
	reg_val = SET_BITS(1, 1, reg_val, 0);
	writel(reg_val, edp_base[sel] + REG_EDP_RESET);
}

void edp_hpd_irq_enable(u32 sel)
{
	writel(0x1, edp_base[sel] + REG_EDP_HPD_INT);
	writel(0x6, edp_base[sel] + REG_EDP_HPD_EN);
}

void edp_hpd_irq_disable(u32 sel)
{
	writel(0x0, edp_base[sel] + REG_EDP_HPD_INT);
	writel(0x0, edp_base[sel] + REG_EDP_HPD_EN);
}

void edp_hpd_enable(u32 sel)
{
	writel(0x8, edp_base[sel] + REG_EDP_HPD_SCALE);
	/* only hpd enable need, irq is not necesssary*/
	writel(0x6, edp_base[sel] + REG_EDP_HPD_EN);
	edp_hpd_irq_enable(sel);
}

void edp_hpd_disable(u32 sel)
{
	writel(0x0, edp_base[sel] + REG_EDP_HPD_SCALE);
	edp_hpd_irq_disable(sel);
}

void edp_hal_irq_handler(u32 sel, struct edp_tx_core *edp_core)
{
	u32 int_event_en = 0;
	u32 int_en = 0;
	bool hpd_plugin;
	bool hpd_plugout;

	int_event_en = readl(edp_base[sel] + REG_EDP_HPD_INT);
	int_en = readl(edp_base[sel] + REG_EDP_HPD_EN);

	/* hpd plugin interrupt */
	if (int_event_en & (1 << 0) && (int_en & (1 << 1))) {
		hpd_plugin = readl(edp_base[sel] + REG_EDP_HPD_PLUG) & (1 << 1);

		if (hpd_plugin == true)
			edp_core->hpd_state = true;

		writel((1 << 1), edp_base[sel] + REG_EDP_HPD_PLUG);
	}

	/* hpd plugout interrupt */
	if (int_event_en & (1 << 0) && (int_en & (1 << 2))) {
		hpd_plugout = readl(edp_base[sel] + REG_EDP_HPD_PLUG) & (1 << 2);

		if (hpd_plugout == true)
			edp_core->hpd_state = false;

		writel((1 << 2), edp_base[sel] + REG_EDP_HPD_PLUG);
	}
}

bool edp_hal_get_hpd_status(u32 sel, struct edp_tx_core *edp_core)
{
	return edp_core->hpd_state;
}

s32 edp_hal_irq_enable(u32 sel, u32 irq_id)
{
	edp_hpd_irq_enable(sel);
	return 0;
}

s32 edp_hal_irq_disable(u32 sel, u32 irq_id)
{
	/*fixme: irq is not need?*/
	//edp_hpd_irq_disable(sel);
	return 0;
}

s32 edp_hal_irq_query(u32 sel)
{
	return 0;
}

s32 edp_hal_irq_clear(u32 sel)
{
	return 0;
}

s32 edp_aux_read(u32 sel, s32 addr, s32 len, char *buf)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i;
	u32 timeout = 0;

	if ((len > 16) || (len <= 0)) {
		EDP_WRN("aux read out of len:%d\n", len);
		return -1;
	}

	memset(buf, 0, 16);

	spin_lock(&aux_lock);
	writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA0);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA1);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA2);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA3);
	writel(0, edp_base[sel] + REG_EDP_PHY_AUX);

	/* aux read request*/
	regval |= (len - 1);
	regval = SET_BITS(8, 20, regval, addr);
	regval = SET_BITS(28, 4, regval, NATIVE_READ);
	EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
	writel(regval, edp_base[sel] + REG_EDP_PHY_AUX);
	udelay(1);
	writel(1, edp_base[sel] + REG_EDP_AUX_START);

	/* wait for AUX_REPLY*/
	while (((readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
		if (timeout >= 50000) {
			EDP_WRN("edp_aux_read wait AUX_REPLY timeout\n");
			spin_unlock(&aux_lock);
			return RET_FAIL;
		}
		timeout++;
	}

	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	regval &= 0xf0;
	if ((regval >> 4) == AUX_REPLY_NACK) {
		EDP_WRN("edp_aux_read recieve AUX_REPLY_NACK!\n");
		spin_unlock(&aux_lock);
		return RET_FAIL;
	}

	/* aux read reply*/
	for (i = 0; i < 4; i++) {
		reg_val[i] = readl(edp_base[sel] + REG_EDP_AUX_DATA0 + i * 0x4);
		udelay(500);
		EDP_LOW_DBG("[%s] result: reg_val[%d] = 0x%x\n", __func__, i, reg_val[i]);
	}

	for (i = 0; i < len; i++) {
		buf[i] = GET_BITS((i % 4) * 8, 8, reg_val[i / 4]);
	}
	spin_unlock(&aux_lock);

	return 0;
}

s32 edp_aux_write(u32 sel, s32 addr, s32 len, char *buf)
{
	u32 reg_val[4];
	u32 regval = 0;
	u32 timeout = 0;
	s32 i;

	if ((len > 16) || (len <= 0)) {
		EDP_WRN("aux read out of len:%d\n", len);
		return -1;
	}

	memset(reg_val, 0, sizeof(reg_val));

	spin_lock(&aux_lock);
	writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA0);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA1);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA2);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA3);
	writel(0, edp_base[sel] + REG_EDP_PHY_AUX);

	for (i = 0; i < len; i++) {
		reg_val[i / 4] = SET_BITS((i % 4) * 8, 8, reg_val[i / 4], buf[i]);
	}

	for (i = 0; i < (1 + ((len - 1) / 4)); i++) {
		writel(reg_val[i], edp_base[sel] + REG_EDP_AUX_DATA0 + (i * 0x4));
		udelay(500);
		EDP_LOW_DBG("[%s]: date: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
	}

	/* aux write request*/
	regval |= (len - 1);
	regval = SET_BITS(8, 20, regval, addr);
	regval = SET_BITS(28, 4, regval, NATIVE_WRITE);
	EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
	writel(regval, edp_base[sel] + REG_EDP_PHY_AUX);

	writel(1, edp_base[sel] + REG_EDP_AUX_START);


	/* wait for AUX_REPLY*/
	while (((readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
		if (timeout >= 50000) {
			EDP_WRN("edp_aux_write wait AUX_REPLY timeout\n");
			spin_unlock(&aux_lock);
			return RET_FAIL;
		}
		timeout++;
	}

	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	regval &= 0xf0;
	if ((regval >> 4) == AUX_REPLY_NACK) {
		EDP_WRN("edp_aux_write recieve AUX_NACK!\n");
		spin_unlock(&aux_lock);
		return RET_FAIL;
	}

	spin_unlock(&aux_lock);

	return 0;
}

s32 edp_aux_i2c_read(u32 sel, s32 addr, s32 len, char *buf)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i;
	u32 timeout = 0;

	if ((len > 16) || (len <= 0)) {
		EDP_WRN("aux read out of len:%d\n", len);
		return -1;
	}

	memset(buf, 0, 16);

	spin_lock(&aux_lock);
	writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA0);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA1);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA2);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA3);
	writel(0, edp_base[sel] + REG_EDP_PHY_AUX);

	/* aux read request*/
	regval |= (len - 1);
	regval = SET_BITS(8, 20, regval, addr);
	regval = SET_BITS(28, 4, regval, AUX_I2C_READ);
	EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
	writel(regval, edp_base[sel] + REG_EDP_PHY_AUX);
	udelay(1);
	writel(1, edp_base[sel] + REG_EDP_AUX_START);

	/* wait for AUX_REPLY*/
	while (((readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
		if (timeout >= 50000) {
			EDP_WRN("edp_aux_i2c_read wait AUX_REPLY timeout\n");
			spin_unlock(&aux_lock);
			return RET_FAIL;
		}
		timeout++;
	}

	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	regval &= 0xf0;
	if ((regval >> 4) == AUX_REPLY_NACK) {
		EDP_WRN("edp_aux_i2c_read recieve AUX_REPLY_NACK!\n");
		spin_unlock(&aux_lock);
		return RET_FAIL;
	}

	/* aux read reply*/
	for (i = 0; i < 4; i++) {
		reg_val[i] = readl(edp_base[sel] + REG_EDP_AUX_DATA0 + i * 0x4);
		EDP_LOW_DBG("[%s]: data: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
	}

	for (i = 0; i < len; i++) {
		buf[i] = GET_BITS((i % 4) * 8, 8, reg_val[i / 4]);
	}
	spin_unlock(&aux_lock);

	return 0;
}

s32 edp_aux_i2c_write(u32 sel, s32 addr, s32 len, char *buf)
{
	u32 reg_val[4];
	u32 regval = 0;
	s32 i;
	u32 timeout = 0;

	if ((len > 16) || (len <= 0)) {
		EDP_WRN("aux read out of len:%d\n", len);
		return -1;
	}

	spin_lock(&aux_lock);
	writel(0, edp_base[sel] + REG_EDP_PHY_AUX);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA0);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA1);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA2);
	writel(0, edp_base[sel] + REG_EDP_AUX_DATA3);
	writel(0, edp_base[sel] + REG_EDP_PHY_AUX);


	for (i = 0; i < len; i++) {
		reg_val[i / 4] = SET_BITS(i % 4, 8, reg_val[i / 4], buf[i]);
	}

	for (i = 0; i < 4; i++) {
		writel(reg_val[i], edp_base[sel] + REG_EDP_AUX_DATA0 + i * 0x4);
		EDP_LOW_DBG("[%s]: data: reg_val[%d] = 0x%x", __func__, i, reg_val[i]);
	}

	/* aux write request*/
	regval |= (len - 1);
	regval = SET_BITS(8, 20, regval, addr);
	regval = SET_BITS(28, 4, regval, AUX_I2C_WRITE);
	EDP_LOW_DBG("[%s] aux_cmd: 0x%x\n", __func__, regval);
	writel(regval, edp_base[sel] + REG_EDP_PHY_AUX);

	writel(1, edp_base[sel] + REG_EDP_AUX_START);

	/* wait for AUX_REPLY*/
	while (((readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT) >> 16) & 0x3) != 0) {
		if (timeout >= 50000) {
			EDP_WRN("edp_aux_i2c_write wait AUX_REPLY timeout\n");
			spin_unlock(&aux_lock);
			return RET_FAIL;
		}
		timeout++;
	}

	regval = readl(edp_base[sel] + REG_EDP_AUX_TIMEOUT);
	regval &= 0xf0;
	if ((regval >> 4) == AUX_REPLY_NACK) {
		EDP_WRN("edp_aux_i2c_write recieve AUX_REPLY_NACK!\n");
		spin_unlock(&aux_lock);
		return RET_FAIL;
	}
	spin_unlock(&aux_lock);

	return 0;
}

s32 edp_hal_aux_read(u32 sel, s32 addr, s32 len, char *buf)
{
	return edp_aux_read(sel, addr, len, buf);
}

s32 edp_hal_aux_write(u32 sel, s32 addr, s32 len, char *buf)
{
	return edp_aux_write(sel, addr, len, buf);
}

s32 edp_hal_aux_i2c_read(u32 sel, s32 addr, s32 len, char *buf)
{
	return edp_aux_i2c_read(sel, addr, len, buf);
}

s32 edp_hal_aux_i2c_write(u32 sel, s32 addr, s32 len, char *buf)
{
	return edp_aux_i2c_write(sel, addr, len, buf);
}


#if 0
s32 edp_link_training1(u32 sel, struct edp_lane_para *para, bool bypass_1st_train)
{
	char g_tx_buf[16];
	char g_rx_buf[16];
	s32 ret = RET_OK;
	u32 to_cnt;
	u32 lane0_sw, lane0_pre;
	u32 lane1_sw, lane1_pre;
	u32 lane2_sw, lane2_pre;
	u32 lane3_sw, lane3_pre;
	u32 lane0_is_pre_max, lane1_is_pre_max,
	    lane2_is_pre_max, lane3_is_pre_max;
	u32 lane0_is_swing_max, lane1_is_swing_max,
	    lane2_is_swing_max, lane3_is_swing_max;

	u32 lane0_sw_old, lane0_pre_old;
	u32 lane1_sw_old, lane1_pre_old;
	u32 lane2_sw_old, lane2_pre_old;
	u32 lane3_sw_old, lane3_pre_old;

	lane0_sw = 0;
	lane0_pre = 0;
	lane1_sw = 0;
	lane1_pre = 0;
	lane2_sw = 0;
	lane2_pre = 0;
	lane3_sw = 0;
	lane3_pre = 0;

	lane0_sw_old = 0;
	lane0_pre_old = 0;
	lane1_sw_old = 0;
	lane1_pre_old = 0;
	lane2_sw_old = 0;
	lane2_pre_old = 0;
	lane3_sw_old = 0;
	lane3_pre_old = 0;

	lane0_is_pre_max = 0;
	lane1_is_pre_max = 0;
	lane2_is_pre_max = 0;
	lane3_is_pre_max = 0;
	lane0_is_swing_max = 0;
	lane1_is_swing_max = 0;
	lane2_is_swing_max = 0;
	lane3_is_swing_max = 0;

	if (para != NULL) {
		if (para->lane0_sw)
			lane0_sw = para->lane0_sw;
		if (para->lane1_sw)
			lane1_sw = para->lane1_sw;
		if (para->lane2_sw)
			lane2_sw = para->lane2_sw;
		if (para->lane3_sw)
			lane3_sw = para->lane3_sw;

		if (para->lane0_pre)
			lane0_pre = para->lane0_pre;
		if (para->lane1_pre)
			lane1_pre = para->lane1_pre;
		if (para->lane2_pre)
			lane2_pre = para->lane2_pre;
		if (para->lane3_pre)
			lane3_pre = para->lane3_pre;

		if (lane0_sw == VOL_SWING_LEVEL_NUM - 1)
			lane0_is_swing_max = 1;
		else
			lane0_is_swing_max = 0;

		if (lane1_sw == VOL_SWING_LEVEL_NUM - 1)
			lane1_is_swing_max = 1;
		else
			lane1_is_swing_max = 0;

		if (lane2_sw == VOL_SWING_LEVEL_NUM - 1)
			lane2_is_swing_max = 1;
		else
			lane2_is_swing_max = 0;

		if (lane3_sw == VOL_SWING_LEVEL_NUM - 1)
			lane3_is_swing_max = 1;
		else
			lane3_is_swing_max = 0;

		if (lane0_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
			lane0_is_pre_max = 1;
		else
			lane0_is_pre_max = 0;

		if (lane1_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
			lane1_is_pre_max = 1;
		else
			lane1_is_pre_max = 0;

		if (lane2_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
			lane2_is_pre_max = 1;
		else
			lane2_is_pre_max = 0;

		if (lane3_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
			lane3_is_pre_max = 1;
		else
			lane3_is_pre_max = 0;
	}

	memset(g_rx_buf, 0, sizeof(g_rx_buf));
	memset(g_tx_buf, 0, sizeof(g_tx_buf));

	if (!bypass_1st_train) {
		edp_hal_set_training_pattern(sel, 1);

		edp_link_lane_para_setting(sel, lane0_sw, lane0_pre, lane1_sw, lane1_pre,
			    lane2_sw, lane2_pre, lane3_sw, lane3_pre);

		mdelay(5);

		/* set pattern 1 with scramble disable */
		g_tx_buf[0] = 0x21;
		g_tx_buf[1] =
		    ((lane0_is_pre_max & 0x1) << 5) | ((lane0_pre & 0x3) << 3) |
		    ((lane0_is_swing_max & 0x1) << 2) | (lane0_sw & 0x3);
		g_tx_buf[2] =
		    ((lane1_is_pre_max & 0x1) << 5) | ((lane1_pre & 0x3) << 3) |
		    ((lane1_is_swing_max & 0x1) << 2) | (lane1_sw & 0x3);
		g_tx_buf[3] =
		    ((lane2_is_pre_max & 0x1) << 5) | ((lane2_pre & 0x3) << 3) |
		    ((lane2_is_swing_max & 0x1) << 2) | (lane2_sw & 0x3);
		g_tx_buf[4] =
		    ((lane3_is_pre_max & 0x1) << 5) | ((lane3_pre & 0x3) << 3) |
		    ((lane3_is_swing_max & 0x1) << 2) | (lane3_sw & 0x3);

		ret = edp_aux_write(sel, 0x0102, g_lane_cnt + 1, g_tx_buf);

		if (ret != RET_OK)
			return RET_FAIL;
	}

	to_cnt = TRAIN_CNT;

	while (1) {
		/* wait for the training finish */
		udelay(training_interval_CR);

		if (g_lane_cnt < 4)
			ret = edp_aux_read(sel, 0x0202, 1, g_rx_buf);
		else
			ret = edp_aux_read(sel, 0x0202, 2, g_rx_buf);
		if (ret != RET_OK)
			return RET_FAIL;

		if (g_lane_cnt < 4) {
			EDP_LOW_DBG("CR training reg[202h]:0x%x\n", g_rx_buf[0]);
		} else {
			EDP_LOW_DBG("CR training reg[202h]:0x%x\n", g_rx_buf[0]);
			EDP_LOW_DBG("CR training reg[203h]:0x%x\n", g_rx_buf[1]);
		}

		if (g_lane_cnt == 1) {
			if ((g_rx_buf[0] & 0x01) == 0x01) {
				recom_training_para.lane0_sw = lane0_sw;
				recom_training_para.lane0_pre = lane0_pre;
				return RET_OK;
			}
		} else if (g_lane_cnt == 2) {
			if ((g_rx_buf[0] & 0x11) == 0x11) {
				recom_training_para.lane0_sw = lane0_sw;
				recom_training_para.lane0_pre = lane0_pre;
				recom_training_para.lane1_sw = lane1_sw;
				recom_training_para.lane1_pre = lane1_pre;
				return RET_OK;
			}
		} else if (g_lane_cnt == 4) {
			if (((g_rx_buf[0] & 0x11) == 0x11) &&
			    ((g_rx_buf[1] & 0x11) == 0x11)) {
				recom_training_para.lane0_sw = lane0_sw;
				recom_training_para.lane0_pre = lane0_pre;
				recom_training_para.lane1_sw = lane1_sw;
				recom_training_para.lane1_pre = lane1_pre;
				recom_training_para.lane2_sw = lane2_sw;
				recom_training_para.lane2_pre = lane2_pre;
				recom_training_para.lane3_sw = lane3_sw;
				recom_training_para.lane3_pre = lane3_pre;
				return RET_OK;
			}
		}

		if (g_lane_cnt < 4)
			ret = edp_aux_read(sel, 0x0206, 1, g_rx_buf);
		else
			ret = edp_aux_read(sel, 0x0206, 2, g_rx_buf);
		if (ret !=  RET_OK)
			return RET_FAIL;
		if (g_lane_cnt < 4)
			EDP_LOW_DBG("CR training reg[206h]:%xh\n", g_rx_buf[0]);
		else {
			EDP_LOW_DBG("CR training reg[206h]:%xh\n", g_rx_buf[0]);
			EDP_LOW_DBG("CR training reg[207h]:%xh\n", g_rx_buf[1]);
		}

		if (g_lane_cnt == 1) {
			lane0_sw = g_rx_buf[0] & 0x3;
			lane0_pre = (g_rx_buf[0] >> 2) & 0x3;

			if ((lane0_sw != lane0_sw_old)) {
				lane0_sw_old = lane0_sw;
				to_cnt = TRAIN_CNT;
			}

			if ((lane0_pre != lane0_pre_old)) {
				lane0_pre_old = lane0_pre;
				to_cnt = TRAIN_CNT;
			}

			if (lane0_sw == VOL_SWING_LEVEL_NUM - 1)
				lane0_is_swing_max = 1;
			else
				lane0_is_swing_max = 0;

			if (lane0_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane0_is_pre_max = 1;
			else
				lane0_is_pre_max = 0;
		} else if (g_lane_cnt == 2) {
			lane0_sw = g_rx_buf[0] & 0x3;
			lane0_pre = (g_rx_buf[0] >> 2) & 0x3;
			lane1_sw = (g_rx_buf[0] >> 4) & 0x3;
			lane1_pre = (g_rx_buf[0] >> 6) & 0x3;

			if ((lane0_sw != lane0_sw_old) ||
			    (lane1_sw != lane1_sw_old)) {

				lane0_sw_old = lane0_sw;
				lane1_sw_old = lane1_sw;
				to_cnt = TRAIN_CNT;
			}

			if ((lane0_pre != lane0_pre_old) ||
			    (lane1_pre != lane1_pre_old)) {
				lane0_pre_old = lane0_pre;
				lane1_pre_old = lane1_pre;
				to_cnt = TRAIN_CNT;
			}

			if (lane0_sw == VOL_SWING_LEVEL_NUM - 1)
				lane0_is_swing_max = 1;
			else
				lane0_is_swing_max = 0;

			if (lane1_sw == VOL_SWING_LEVEL_NUM - 1)
				lane1_is_swing_max = 1;
			else
				lane1_is_swing_max = 0;

			if (lane0_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane0_is_pre_max = 1;
			else
				lane0_is_pre_max = 0;

			if (lane1_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane1_is_pre_max = 1;
			else
				lane1_is_pre_max = 0;
		} else if (g_lane_cnt == 4) {
			lane0_sw = g_rx_buf[0] & 0x3;
			lane0_pre = (g_rx_buf[0] >> 2) & 0x3;
			lane1_sw = (g_rx_buf[0] >> 4) & 0x3;
			lane1_pre = (g_rx_buf[0] >> 6) & 0x3;

			lane2_sw = g_rx_buf[1] & 0x3;
			lane2_pre = (g_rx_buf[1] >> 2) & 0x3;
			lane3_sw = (g_rx_buf[1] >> 4) & 0x3;
			lane3_pre = (g_rx_buf[1] >> 6) & 0x3;

			if ((lane0_sw != lane0_sw_old) ||
			    (lane1_sw != lane1_sw_old) ||
			    (lane2_sw != lane2_sw_old) ||
			    (lane3_sw != lane3_sw_old)) {
				lane0_sw_old = lane0_sw;
				lane1_sw_old = lane1_sw;
				lane2_sw_old = lane2_sw;
				lane3_sw_old = lane3_sw;
				to_cnt = TRAIN_CNT;
			}

			if ((lane0_pre != lane0_pre_old) ||
			    (lane1_pre != lane1_pre_old) ||
			    (lane2_pre != lane2_pre_old) ||
			    (lane3_pre != lane3_pre_old)) {
				lane0_pre_old = lane0_pre;
				lane1_pre_old = lane1_pre;
				lane2_pre_old = lane2_pre;
				lane3_pre_old = lane3_pre;
				to_cnt = TRAIN_CNT;
			}

			if (lane0_sw == VOL_SWING_LEVEL_NUM - 1)
				lane0_is_swing_max = 1;
			else
				lane0_is_swing_max = 0;

			if (lane1_sw == VOL_SWING_LEVEL_NUM - 1)
				lane1_is_swing_max = 1;
			else
				lane1_is_swing_max = 0;

			if (lane2_sw == VOL_SWING_LEVEL_NUM - 1)
				lane2_is_swing_max = 1;
			else
				lane2_is_swing_max = 0;

			if (lane3_sw == VOL_SWING_LEVEL_NUM - 1)
				lane3_is_swing_max = 1;
			else
				lane3_is_swing_max = 0;

			if (lane0_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane0_is_pre_max = 1;
			else
				lane0_is_pre_max = 0;

			if (lane1_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane1_is_pre_max = 1;
			else
				lane1_is_pre_max = 0;

			if (lane2_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane2_is_pre_max = 1;
			else
				lane2_is_pre_max = 0;

			if (lane3_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane3_is_pre_max = 1;
			else
				lane3_is_pre_max = 0;
		}

		to_cnt--;

		if (to_cnt == 0) {
			return RET_FAIL;
		}

		memset(g_tx_buf, 0, sizeof(g_tx_buf));
		/* set pattern 1 with scramble disable */
		g_tx_buf[0] = 0x21;
		/* set pattern 1 with max swing and emphasis 0x13 */
		g_tx_buf[1] =
		    ((lane0_is_pre_max & 0x1) << 5) | ((lane0_pre & 0x3) << 3) |
		    ((lane0_is_swing_max & 0x1) << 2) | (lane0_sw & 0x3);
		g_tx_buf[2] =
		    ((lane1_is_pre_max & 0x1) << 5) | ((lane1_pre & 0x3) << 3) |
		    ((lane1_is_swing_max & 0x1) << 2) | (lane1_sw & 0x3);
		g_tx_buf[3] =
		    ((lane2_is_pre_max & 0x1) << 5) | ((lane2_pre & 0x3) << 3) |
		    ((lane2_is_swing_max & 0x1) << 2) | (lane2_sw & 0x3);
		g_tx_buf[4] =
		    ((lane3_is_pre_max & 0x1) << 5) | ((lane3_pre & 0x3) << 3) |
		    ((lane3_is_swing_max & 0x1) << 2) | (lane3_sw & 0x3);

		/*set training pattern*/
		edp_hal_set_training_pattern(sel, 1);

		edp_link_lane_para_setting(sel, lane0_sw, lane0_pre, lane1_sw, lane1_pre,
			    lane2_sw, lane2_pre, lane3_sw, lane3_pre);
		mdelay(5);

		ret = edp_aux_write(sel, 0x0102, g_lane_cnt + 1, g_tx_buf);

		if (ret != RET_OK)
			return RET_FAIL;
	}
}

s32 edp_link_training2(u32 sel, struct edp_lane_para *para, bool bypass_1st_train)
{
	char g_tx_buf[16];
	char g_rx_buf[16];
	u32 to_cnt;
	s32 ret;
	s32 eq_end = 0;
	u32 lane_cnt = para->lane_cnt;

	u32 lane0_sw, lane0_pre;
	u32 lane1_sw, lane1_pre;
	u32 lane2_sw, lane2_pre;
	u32 lane3_sw, lane3_pre;
	u32 lane0_is_pre_max, lane1_is_pre_max, lane2_is_pre_max,
	    lane3_is_pre_max;
	u32 lane0_is_swing_max, lane1_is_swing_max, lane2_is_swing_max,
	    lane3_is_swing_max;

	u32 lane0_sw_old, lane0_pre_old;
	u32 lane1_sw_old, lane1_pre_old;
	u32 lane2_sw_old, lane2_pre_old;
	u32 lane3_sw_old, lane3_pre_old;

	lane0_sw = 0;
	lane0_pre = 0;
	lane1_sw = 0;
	lane1_pre = 0;
	lane2_sw = 0;
	lane2_pre = 0;
	lane3_sw = 0;
	lane3_pre = 0;

	lane0_sw_old = 0;
	lane0_pre_old = 0;
	lane1_sw_old = 0;
	lane1_pre_old = 0;
	lane2_sw_old = 0;
	lane2_pre_old = 0;
	lane3_sw_old = 0;
	lane3_pre_old = 0;

	lane0_is_pre_max = 0;
	lane1_is_pre_max = 0;
	lane2_is_pre_max = 0;
	lane3_is_pre_max = 0;
	lane0_is_swing_max = 0;
	lane1_is_swing_max = 0;
	lane2_is_swing_max = 0;
	lane3_is_swing_max = 0;

	if (para != NULL) {
		if (para->lane0_sw)
			lane0_sw = para->lane0_sw;
		if (para->lane1_sw)
			lane1_sw = para->lane1_sw;
		if (para->lane2_sw)
			lane2_sw = para->lane2_sw;
		if (para->lane3_sw)
			lane3_sw = para->lane3_sw;

		if (para->lane0_pre)
			lane0_pre = para->lane0_pre;
		if (para->lane1_pre)
			lane1_pre = para->lane1_pre;
		if (para->lane2_pre)
			lane2_pre = para->lane2_pre;
		if (para->lane3_pre)
			lane3_pre = para->lane3_pre;

		if (lane0_sw == VOL_SWING_LEVEL_NUM - 1)
			lane0_is_swing_max = 1;
		else
			lane0_is_swing_max = 0;

		if (lane1_sw == VOL_SWING_LEVEL_NUM - 1)
			lane1_is_swing_max = 1;
		else
			lane1_is_swing_max = 0;

		if (lane2_sw == VOL_SWING_LEVEL_NUM - 1)
			lane2_is_swing_max = 1;
		else
			lane2_is_swing_max = 0;

		if (lane3_sw == VOL_SWING_LEVEL_NUM - 1)
			lane3_is_swing_max = 1;
		else
			lane3_is_swing_max = 0;

		if (lane0_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
			lane0_is_pre_max = 1;
		else
			lane0_is_pre_max = 0;

		if (lane1_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
			lane1_is_pre_max = 1;
		else
			lane1_is_pre_max = 0;

		if (lane2_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
			lane2_is_pre_max = 1;
		else
			lane2_is_pre_max = 0;

		if (lane3_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
			lane3_is_pre_max = 1;
		else
			lane3_is_pre_max = 0;
	}

	memset(g_rx_buf, 0, sizeof(g_rx_buf));
	memset(g_tx_buf, 0, sizeof(g_tx_buf));

	if (!bypass_1st_train) {
		to_cnt = TRAIN_CNT;
		edp_hal_set_training_pattern(sel, 2);

		edp_link_lane_para_setting(sel, lane0_sw, lane0_pre, lane1_sw, lane1_pre,
			    lane2_sw, lane2_pre, lane3_sw, lane3_pre);

		mdelay(5);

		/* fixme: should be 0x26? */
		/* set pattern 1 with scramble disable */
		g_tx_buf[0] = 0x22;
		g_tx_buf[1] =
		    ((lane0_is_pre_max & 0x1) << 5) | ((lane0_pre & 0x3) << 3) |
		    ((lane0_is_swing_max & 0x1) << 2) | (lane0_sw & 0x3);
		g_tx_buf[2] =
		    ((lane1_is_pre_max & 0x1) << 5) | ((lane1_pre & 0x3) << 3) |
		    ((lane1_is_swing_max & 0x1) << 2) | (lane1_sw & 0x3);
		g_tx_buf[3] =
		    ((lane2_is_pre_max & 0x1) << 5) | ((lane2_pre & 0x3) << 3) |
		    ((lane2_is_swing_max & 0x1) << 2) | (lane2_sw & 0x3);
		g_tx_buf[4] =
		    ((lane3_is_pre_max & 0x1) << 5) | ((lane3_pre & 0x3) << 3) |
		    ((lane3_is_swing_max & 0x1) << 2) | (lane3_sw & 0x3);

		ret = edp_aux_write(sel, 0x0102, g_lane_cnt + 1, g_tx_buf);

		if (ret != RET_OK)
			return RET_FAIL;
	} else
		to_cnt = TRAIN_CNT + 1;


	while (1) {
		if (training_interval_EQ > 1) {
			/*wait for the training finish*/
			udelay(training_interval_EQ);
		} else {
			udelay(400);
		}
		ret = edp_aux_read(sel, 0x0202, 3, g_rx_buf);
		if (ret == -1)
			return RET_FAIL;

		if (lane_cnt < 4) {
			EDP_LOW_DBG("EQ training reg[202h]:0x%x\n", g_rx_buf[0]);
			EDP_LOW_DBG("EQ training reg[204h]:0x%x\n", g_rx_buf[2]);
		} else {
			EDP_LOW_DBG("EQ training reg[202h]:0x%x\n", g_rx_buf[0]);
			EDP_LOW_DBG("EQ training reg[203h]:0x%x\n", g_rx_buf[1]);
			EDP_LOW_DBG("EQ training reg[204h]:0x%x\n", g_rx_buf[2]);
		}

		if (lane_cnt == 1) {
			if ((g_rx_buf[0] & 0x01) != 0x01) {
				EDP_LOW_DBG("CR is not done before EQ training\n");
				return RET_FAIL;
			}
		} else if (lane_cnt == 2) {
			if (((g_rx_buf[0] & 0x11) != 0x11)) {
				EDP_LOW_DBG("CR is not done before EQ training\n");
				return RET_FAIL;
			}
		} else if (lane_cnt == 4) {
			if (((g_rx_buf[0] & 0x11) != 0x11) &&
			    ((g_rx_buf[1] & 0x11) != 0x11)) {
				EDP_LOW_DBG("CR is not done before EQ training\n");
				return RET_FAIL;
			}
		}

		if (g_rx_buf[2] & 0x01) {
			if (lane_cnt == 1) {
				if ((g_rx_buf[0] & 0x07) == 0x07) {
					eq_end = 1;
				}
			} else if (lane_cnt == 2) {
				if (((g_rx_buf[0] & 0x77) == 0x77)) {
					eq_end = 1;
				}
			} else if (lane_cnt == 4) {
				if (((g_rx_buf[0] & 0x77) == 0x77) &&
				    ((g_rx_buf[1] & 0x77) == 0x77)) {
					eq_end = 1;
				}
			}

			if (eq_end == 1) {
				memset(g_tx_buf, 0, sizeof(g_tx_buf));
				g_tx_buf[0] = 0x00; /* 102 indicate the end of training*/
				g_tx_buf[1] = 0x00; /* 103 */
				g_tx_buf[2] = 0x00; /* 104 */
				g_tx_buf[3] = 0x00; /* 105 */
				g_tx_buf[4] = 0x00; /* 106 */
				g_tx_buf[5] = 0x00; /* 107 */
				g_tx_buf[6] = 0x01; /* 108 */
				g_tx_buf[7] = 0x00; /* 109 */
				g_tx_buf[8] = 0x00; /* 10a */
				g_tx_buf[9] = 0x00;	/* 10b */
				g_tx_buf[10] = 0x00;	/* 10c */
				g_tx_buf[11] = 0x00;	/* 10d */
				g_tx_buf[12] = 0x00;	/* 10e */
				ret = edp_aux_write(sel, 0x0102, 7, g_tx_buf);
				if (ret != RET_OK)
					return RET_FAIL;

				return RET_OK;
			}
		}

		if (lane_cnt < 4)
			ret = edp_aux_read(sel, 0x0206, 1, g_rx_buf);
		else
			ret = edp_aux_read(sel, 0x0206, 2, g_rx_buf);
		if (ret == -1)
			return RET_FAIL;
		if (lane_cnt < 4)
			EDP_LOW_DBG("EQ training reg[206h]:%xh\n", g_rx_buf[0]);
		else {
			EDP_LOW_DBG("EQ training reg[206h]:%xh\n", g_rx_buf[0]);
			EDP_LOW_DBG("EQ training reg[207h]:%xh\n", g_rx_buf[1]);
		}

		if (lane_cnt == 1) {
			lane0_sw = g_rx_buf[0] & 0x3;
			lane0_pre = (g_rx_buf[0] >> 2) & 0x3;

			if ((lane0_sw != lane0_sw_old)) {
				lane0_sw_old = lane0_sw;
				to_cnt = TRAIN_CNT;
			}

			if ((lane0_pre != lane0_pre_old)) {
				lane0_pre_old = lane0_pre;
				to_cnt = TRAIN_CNT;
			}

			if (lane0_sw == VOL_SWING_LEVEL_NUM - 1)
				lane0_is_swing_max = 1;
			else
				lane0_is_swing_max = 0;

			if (lane0_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane0_is_pre_max = 1;
			else
				lane0_is_pre_max = 0;
		} else if (lane_cnt == 2) {
			lane0_sw = g_rx_buf[0] & 0x3;
			lane0_pre = (g_rx_buf[0] >> 2) & 0x3;
			lane1_sw = (g_rx_buf[0] >> 4) & 0x3;
			lane1_pre = (g_rx_buf[0] >> 6) & 0x3;

			if ((lane0_sw != lane0_sw_old) ||
			    (lane1_sw != lane1_sw_old)) {

				lane0_sw_old = lane0_sw;
				lane1_sw_old = lane1_sw;
				to_cnt = TRAIN_CNT;
			}

			if ((lane0_pre != lane0_pre_old) ||
			    (lane1_pre != lane1_pre_old)) {
				lane0_pre_old = lane0_pre;
				lane1_pre_old = lane1_pre;
				to_cnt = TRAIN_CNT;
			}

			if (lane0_sw == VOL_SWING_LEVEL_NUM - 1)
				lane0_is_swing_max = 1;
			else
				lane0_is_swing_max = 0;

			if (lane1_sw == VOL_SWING_LEVEL_NUM - 1)
				lane1_is_swing_max = 1;
			else
				lane1_is_swing_max = 0;

			if (lane0_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane0_is_pre_max = 1;
			else
				lane0_is_pre_max = 0;

			if (lane1_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane1_is_pre_max = 1;
			else
				lane1_is_pre_max = 0;
		} else if (lane_cnt == 4) {
			lane0_sw = g_rx_buf[0] & 0x3;
			lane0_pre = (g_rx_buf[0] >> 2) & 0x3;
			lane1_sw = (g_rx_buf[0] >> 4) & 0x3;
			lane1_pre = (g_rx_buf[0] >> 6) & 0x3;

			lane2_sw = g_rx_buf[1] & 0x3;
			lane2_pre = (g_rx_buf[1] >> 2) & 0x3;
			lane3_sw = (g_rx_buf[1] >> 4) & 0x3;
			lane3_pre = (g_rx_buf[1] >> 6) & 0x3;

			if ((lane0_sw != lane0_sw_old) ||
			    (lane1_sw != lane1_sw_old) ||
			    (lane2_sw != lane2_sw_old) ||
			    (lane3_sw != lane3_sw_old)) {
				lane0_sw_old = lane0_sw;
				lane1_sw_old = lane1_sw;
				lane2_sw_old = lane2_sw;
				lane3_sw_old = lane3_sw;
				to_cnt = TRAIN_CNT;
			}

			if ((lane0_pre != lane0_pre_old) ||
			    (lane1_pre != lane1_pre_old) ||
			    (lane2_pre != lane2_pre_old) ||
			    (lane3_pre != lane3_pre_old)) {
				lane0_pre_old = lane0_pre;
				lane1_pre_old = lane1_pre;
				lane2_pre_old = lane2_pre;
				lane3_pre_old = lane3_pre;
				to_cnt = TRAIN_CNT;
			}

			if (lane0_sw == VOL_SWING_LEVEL_NUM - 1)
				lane0_is_swing_max = 1;
			else
				lane0_is_swing_max = 0;

			if (lane1_sw == VOL_SWING_LEVEL_NUM - 1)
				lane1_is_swing_max = 1;
			else
				lane1_is_swing_max = 0;

			if (lane2_sw == VOL_SWING_LEVEL_NUM - 1)
				lane2_is_swing_max = 1;
			else
				lane2_is_swing_max = 0;

			if (lane3_sw == VOL_SWING_LEVEL_NUM - 1)
				lane3_is_swing_max = 1;
			else
				lane3_is_swing_max = 0;

			if (lane0_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane0_is_pre_max = 1;
			else
				lane0_is_pre_max = 0;

			if (lane1_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane1_is_pre_max = 1;
			else
				lane1_is_pre_max = 0;

			if (lane2_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane2_is_pre_max = 1;
			else
				lane2_is_pre_max = 0;

			if (lane3_pre == PRE_EMPHASIS_LEVEL_NUM - 1)
				lane3_is_pre_max = 1;
			else
				lane3_is_pre_max = 0;
		}

		to_cnt--;

		if (to_cnt == 0) {
			return RET_FAIL;
		}

		memset(g_tx_buf, 0, sizeof(g_tx_buf));
		g_tx_buf[0] = 0x22; /* set pattern 2 with scramble disable */
		/* set pattern 1 with max swing and emphasis 0x13 */
		g_tx_buf[1] =
		    ((lane0_is_pre_max & 0x1) << 5) | ((lane0_pre & 0x3) << 3) |
		    ((lane0_is_swing_max & 0x1) << 2) | (lane0_sw & 0x3);
		g_tx_buf[2] =
		    ((lane1_is_pre_max & 0x1) << 5) | ((lane1_pre & 0x3) << 3) |
		    ((lane1_is_swing_max & 0x1) << 2) | (lane1_sw & 0x3);
		g_tx_buf[3] =
		    ((lane2_is_pre_max & 0x1) << 5) | ((lane2_pre & 0x3) << 3) |
		    ((lane2_is_swing_max & 0x1) << 2) | (lane2_sw & 0x3);
		g_tx_buf[4] =
		    ((lane3_is_pre_max & 0x1) << 5) | ((lane3_pre & 0x3) << 3) |
		    ((lane3_is_swing_max & 0x1) << 2) | (lane3_sw & 0x3);

		edp_hal_set_training_pattern(sel, 2);

		edp_link_lane_para_setting(sel, lane0_sw, lane0_pre, lane1_sw, lane1_pre,
			    lane2_sw, lane2_pre, lane3_sw, lane3_pre);

		mdelay(20);

		ret = edp_aux_write(sel, 0x0102, g_lane_cnt + 1, g_tx_buf);
		if (ret != RET_OK)
			return RET_FAIL;
	}
}
#endif


s32 edp_transfer_unit_config(u32 sel, u32 lane_cnt, u64 bit_rate, u32 pixel_clk)
{
	u32 reg_val;
	u32 pack_data_rate;
	u32 valid_symbol;
	u32 hblank;
	u32 bandwidth;
	u32 pre_div = 1000;

	reg_val = readl(edp_base[sel] + REG_EDP_HACTIVE_BLANK);
	hblank = GET_BITS(2, 14, reg_val);

	/*
	 * avg valid syobol per TU: pack_data_rate / bandwidth * LS_PER_TU
	 * pack_data_rate = (bpp / 8bit) * pix_clk / lane_cnt (1 symbol is 8 bit)
	 */

	pixel_clk = pixel_clk / 1000;
	bandwidth = bit_rate / 10000000;

	pack_data_rate = ((g_bpp / 8)  * pixel_clk) / lane_cnt;
	EDP_LOW_DBG("[edp_transfer_unit]: pack_data_rate:%d\n", pack_data_rate);
	valid_symbol = LS_PER_TU * (pack_data_rate / bandwidth);

	if (valid_symbol > (62 * pre_div)) {
		EDP_ERR("valid symbol now: %d, should less than 62\n", (valid_symbol / pre_div));
		EDP_ERR("Try to enlarge lane count or lane rate!\n");
		return RET_FAIL;
	}

	EDP_LOW_DBG("[edp_transfer_unit]: g_bpp:%d valid_symbol:%d\n", g_bpp, valid_symbol);

	reg_val = readl(edp_base[sel] + REG_EDP_FRAME_UNIT);
	reg_val = SET_BITS(0, 7, reg_val, valid_symbol / pre_div);
	reg_val = SET_BITS(16, 4, reg_val, (valid_symbol % pre_div) / 100);

	if ((valid_symbol / pre_div) < 6)
		reg_val = SET_BITS(7, 7, reg_val, 32);
	else {
		if (hblank < 80)
			reg_val = SET_BITS(7, 7, reg_val, 12);
		else
			reg_val = SET_BITS(7, 7, reg_val, 16);
	}
	writel(reg_val, edp_base[sel] + REG_EDP_FRAME_UNIT);

	return RET_OK;
}


void edp_set_link_clk_cyc(u32 sel, u32 lane_cnt, u64 bit_rate, u32 pixel_clk)
{
	u32 reg_val;
	u32 hblank;
	u32 symbol_clk;
	u32 link_cyc;

	/*hblank_link_cyc = hblank * (symbol_clk / 4) / pixclk*/
	reg_val = readl(edp_base[sel] + REG_EDP_HACTIVE_BLANK);
	hblank = GET_BITS(2, 14, reg_val);

	symbol_clk = bit_rate / 10000000;


	link_cyc = 1000 * hblank * (symbol_clk / lane_cnt) / (pixel_clk / 1000);
	EDP_LOW_DBG("link_cyc:%d hblank:%d symbol_clk:%d pixel_clk:%d\n", link_cyc, hblank, symbol_clk, pixel_clk);

	reg_val = readl(edp_base[sel] + REG_EDP_HBLANK_LINK_CYC);
	reg_val = SET_BITS(0, 16, reg_val, link_cyc);
	writel(reg_val, edp_base[sel] + REG_EDP_HBLANK_LINK_CYC);
}

s32 edp_hal_init_early(u32 sel)
{
	spin_lock_init(&aux_lock);

	return RET_OK;
}

s32 edp_hal_phy_init(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret = 0;

	/* reserved for debug */
	//ret = edp_bist_test(sel, edp_core);
	if (ret < 0)
		return ret;
	edp_phy_soft_reset(sel);
	edp_hpd_enable(sel);
	edp_mode_init(sel, 1);

	return ret;
}

void edp_hal_set_reg_base(u32 sel, uintptr_t base)
{
	edp_base[sel] = (void __iomem *)(base);
}


s32 edp_hal_enable(u32 sel, struct edp_tx_core *edp_core)
{
	u64 bit_rate;

	bit_rate = edp_core->lane_para.bit_rate;

	edp_aux_16m_config(sel, bit_rate);

	edp_corepll_config(sel, bit_rate);

	mdelay(100);

	return RET_OK;
}

s32 edp_hal_disable(u32 sel, struct edp_tx_core *edp_core)
{
	edp_video_stream_disable(sel);

	return 0;
}


void edp_hal_lane_config(u32 sel, struct edp_tx_core *edp_core)
{
	u32 lane0_sw, lane0_pre;
	u32 lane1_sw, lane1_pre;
	u32 lane2_sw, lane2_pre;
	u32 lane3_sw, lane3_pre;
	u32 lane_cnt;
	u64 bit_rate;

	lane0_sw = edp_core->lane_para.lane0_sw;
	lane1_sw = edp_core->lane_para.lane1_sw;
	lane2_sw = edp_core->lane_para.lane2_sw;
	lane3_sw = edp_core->lane_para.lane3_sw;

	lane0_pre = edp_core->lane_para.lane0_pre;
	lane1_pre = edp_core->lane_para.lane1_pre;
	lane2_pre = edp_core->lane_para.lane2_pre;
	lane3_pre = edp_core->lane_para.lane3_pre;

	lane_cnt = edp_core->lane_para.lane_cnt;
	bit_rate = edp_core->lane_para.bit_rate;

	edp_lane_config(sel, lane_cnt, bit_rate);
	edp_link_lane_para_setting(sel, lane0_sw, lane0_pre, lane1_sw, lane1_pre,
			    lane2_sw, lane2_pre, lane3_sw, lane3_pre);
	edp_corepll_config(sel, bit_rate);
}

s32 edp_read_edid(u32 sel, struct edid *edid)
{
	s32 i;
	s32 ret;

	for (i = 0; i < (EDID_LENGTH / 16); i++) {
		ret = edp_aux_i2c_read(sel, EDID_ADDR, 16, (char *)(edid) + (i * 16));
		if (ret < 0)
			return ret;

	}
	for (i = 0; i < 128; i++)
		EDP_EDID_DBG("edid[%d] = 0x%x\n", i, *((char *)(edid) + i));

	return 0;
}

s32 edp_hal_read_edid(u32 sel, struct edid *edid)
{
	char g_tx_buf[16];

	memset(g_tx_buf, 0, sizeof(g_tx_buf));

	edp_aux_i2c_write(sel, EDID_ADDR, 1, &g_tx_buf[0]);

	return edp_read_edid(sel, edid);
}

s32 edp_hal_read_edid_ext(u32 sel, u32 block_cnt, struct edid *edid)
{
	s32 i, j, ret;
	struct edid *edid_ext;

	for (i = 1; i < block_cnt; i++) {
		edid_ext = edid + i;
		ret = edp_read_edid(sel, edid_ext);
		if (ret < 0)
			return ret;

		for (j = 0; j < 128; j++) {
			EDP_EDID_DBG("edid_ext[%d] = 0x%x\n", j, *((char *)(edid_ext) + j));
		}
	}

	return 0;
}

void edp_hal_set_video_timing(u32 sel, struct disp_video_timings *timings)
{
	u32 reg_val;

	/*hsync/vsync polarity setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_SYNC_POLARITY);
	reg_val = SET_BITS(1, 1, reg_val, timings->hor_sync_polarity);
	reg_val = SET_BITS(0, 1, reg_val, timings->ver_sync_polarity);
	writel(reg_val, edp_base[sel] + REG_EDP_SYNC_POLARITY);

	/*h/vactive h/vblank setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_HACTIVE_BLANK);
	reg_val = SET_BITS(16, 16, reg_val, timings->x_res);
	reg_val = SET_BITS(2, 14, reg_val, (timings->hor_total_time - timings->x_res));
	writel(reg_val, edp_base[sel] + REG_EDP_HACTIVE_BLANK);

	reg_val = readl(edp_base[sel] + REG_EDP_VACTIVE_BLANK);
	reg_val = SET_BITS(0, 16, reg_val, timings->y_res);
	reg_val = SET_BITS(16, 16, reg_val, (timings->ver_total_time - timings->y_res));
	writel(reg_val, edp_base[sel] + REG_EDP_VACTIVE_BLANK);

	/*h/vstart setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_SYNC_START);
	reg_val = SET_BITS(0, 16, reg_val, (timings->hor_sync_time + timings->hor_back_porch));
	reg_val = SET_BITS(16, 16, reg_val, (timings->ver_sync_time + timings->ver_back_porch));
	writel(reg_val, edp_base[sel] + REG_EDP_SYNC_START);

	/*hs/vswidth  h/v_front_porch setting*/
	reg_val = readl(edp_base[sel] + REG_EDP_HSW_FRONT_PORCH);
	reg_val = SET_BITS(16, 16, reg_val, timings->hor_sync_time);
	reg_val = SET_BITS(0, 16, reg_val, timings->hor_front_porch);
	writel(reg_val, edp_base[sel] + REG_EDP_HSW_FRONT_PORCH);

	reg_val = readl(edp_base[sel] + REG_EDP_VSW_FRONT_PORCH);
	reg_val = SET_BITS(16, 16, reg_val, timings->ver_sync_time);
	reg_val = SET_BITS(0, 16, reg_val, timings->ver_front_porch);
	writel(reg_val, edp_base[sel] + REG_EDP_VSW_FRONT_PORCH);
}


s32 edp_hal_set_video_mode(u32 sel, struct edp_tx_core *edp_core)
{
	s32 ret;

	struct disp_video_timings *tmgs;
	u32 pixel_clk;
	u32 colordepth;
	u32 color_fmt;
	u64 bit_rate;
	u32 video_map;
	u32 lane_cnt;

	tmgs = &edp_core->timings;
	pixel_clk = tmgs->pixel_clk;
	colordepth = edp_core->lane_para.colordepth;
	color_fmt = edp_core->lane_para.color_fmt;
	bit_rate = edp_core->lane_para.bit_rate;
	lane_cnt = edp_core->lane_para.lane_cnt;

	/* 0:RGB  1:YUV444  2:YUV422*/
	if (color_fmt == 0) {
		switch (colordepth) {
		case 6:
			video_map = RGB_6BIT;
			break;
		case 10:
			video_map = RGB_10BIT;
			break;
		case 12:
			video_map = RGB_12BIT;
			break;
		case 16:
			video_map = RGB_16BIT;
			break;
		case 8:
		default:
			video_map = RGB_8BIT;
			break;
		}
	} else if (color_fmt == 1) {
		switch (colordepth) {
		case 10:
			video_map = YCBCR444_10BIT;
			break;
		case 12:
			video_map = YCBCR444_12BIT;
			break;
		case 16:
			video_map = YCBCR444_16BIT;
			break;
		case 8:
		default:
			video_map = YCBCR444_8BIT;
			break;
		}
	} else if (color_fmt == 2) {
		switch (colordepth) {
		case 10:
			video_map = YCBCR422_10BIT;
			break;
		case 12:
			video_map = YCBCR422_12BIT;
			break;
		case 16:
			video_map = YCBCR422_16BIT;
			break;
		case 8:
		default:
			video_map = YCBCR422_8BIT;
			break;
		}
	} else {
		EDP_ERR("color format is not support!");
		return RET_FAIL;
	}


	edp_hal_set_video_timing(sel, tmgs);
	edp_pixpll_cfg(sel, pixel_clk);
	edp_set_input_video_mapping(sel, (enum edp_video_mapping_e) video_map);
	ret = edp_transfer_unit_config(sel, lane_cnt, bit_rate, pixel_clk);
	if (ret < 0)
		return ret;
	edp_set_link_clk_cyc(sel, lane_cnt, bit_rate, pixel_clk);

	return RET_OK;
}

s32 edp_hal_audio_enable(u32 sel)
{
	edp_audio_timestamp_hblank_setting(sel, true);
	edp_audio_timestamp_vblank_setting(sel, true);
	edp_audio_stream_hblank_setting(sel, true);
	edp_audio_stream_vblank_setting(sel, true);
	edp_audio_soft_reset(sel);

	return RET_OK;
}

s32 edp_hal_audio_disable(u32 sel)
{
	edp_audio_timestamp_hblank_setting(sel, false);
	edp_audio_timestamp_vblank_setting(sel, false);
	edp_audio_stream_hblank_setting(sel, false);
	edp_audio_stream_vblank_setting(sel, false);

	return RET_OK;
}


s32 edp_hal_audio_set_para(u32 sel, edp_audio_t *para)
{
	edp_audio_interface_config(sel, para->interface);
	edp_audio_channel_config(sel, para->chn_cnt);
	edp_audio_mute_config(sel, para->mute);
	edp_audio_data_width_config(sel, para->data_width);

	return RET_OK;
}

s32 edp_hal_ssc_enable(u32 sel, bool enable)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	if (enable)
		reg_val = SET_BITS(21, 1, reg_val, 0);
	else
		reg_val = SET_BITS(21, 1, reg_val, 1);
	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	return RET_OK;
}

bool edp_hal_ssc_is_enabled(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = GET_BITS(21, 1, reg_val);

	if (!reg_val)
		return true;

	return false;
}

s32 edp_hal_ssc_get_mode(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);
	reg_val = GET_BITS(20, 1, reg_val);

	return reg_val;
}

s32 edp_hal_ssc_set_mode(u32 sel, u32 mode)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	switch (mode) {
	case SSC_CENTER_MODE:
		reg_val = SET_BITS(20, 1, reg_val, 0);
		break;
	case SSC_DOWNSPR_MODE:
	default:
		reg_val = SET_BITS(20, 1, reg_val, 1);
		break;
	}

	writel(reg_val, edp_base[sel] + REG_EDP_ANA_PLL_FBDIV);

	return RET_OK;
}


s32 edp_hal_psr_enable(u32 sel, bool enable)
{
	EDP_ERR("psr isn't support\n");
	return RET_FAIL;
}

bool edp_hal_psr_is_enabled(u32 sel)
{
	EDP_ERR("psr isn't support\n");
	return false;
}

s32 edp_hal_get_color_fmt(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_VIDEO_STREAM_EN);

	reg_val = GET_BITS(16, 5, reg_val);

	switch (reg_val) {
	case 0:
		return RGB_6BIT;
	case 1:
		return RGB_8BIT;
	case 2:
		return RGB_10BIT;
	case 3:
		return RGB_12BIT;
	case 4:
		return RGB_16BIT;
	case 5:
		return YCBCR444_8BIT;
	case 6:
		return YCBCR444_10BIT;
	case 7:
		return YCBCR444_12BIT;
	case 8:
		return YCBCR444_16BIT;
	case 9:
		return YCBCR422_8BIT;
	case 10:
		return YCBCR422_10BIT;
	case 11:
		return YCBCR422_12BIT;
	case 12:
		return YCBCR422_16BIT;
	}

	return RET_FAIL;
}


s32 edp_hal_get_pixclk(u32 sel)
{
	u32 reg_val;
	u32 fb_div;
	u32 pre_div;
	u32 pll_divb;
	u32 pll_divc;
	u32 pixclk;

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_FBDIV);
	fb_div = GET_BITS(24, 8, reg_val);
	pre_div = GET_BITS(8, 6, reg_val);

	reg_val = readl(edp_base[sel] + REG_EDP_ANA_PIXPLL_DIV);
	pll_divc = GET_BITS(16, 5, reg_val);
	reg_val = GET_BITS(8, 2, reg_val);
	if (reg_val == 1)
		pll_divb = 2;
	else if (reg_val == 2)
		pll_divb = 3;
	else if (reg_val == 3)
		pll_divb = 5;
	else
		pll_divb = 1;

	pixclk = (24 * fb_div) / pre_div;
	pixclk = pixclk / (2 * pll_divb * pll_divc);

	return pixclk * 1000000;
}

s32 edp_hal_get_train_pattern(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);

	reg_val = GET_BITS(0, 1, reg_val);

	return reg_val;
}

s32 edp_hal_get_lane_para(u32 sel, struct edp_lane_para *tmp_lane_para)
{
	u32 reg_val;
	u32 regval;

	/* bit rate */
	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	regval = GET_BITS(26, 3, reg_val);
	if (regval == 1)
		tmp_lane_para->bit_rate = 2160000000;
	else if (regval == 2)
		tmp_lane_para->bit_rate = 2430000000;
	else {
		regval = GET_BITS(4, 2, reg_val);
		if (regval == 0)
			tmp_lane_para->bit_rate = 1620000000;
		else if (regval == 1)
			tmp_lane_para->bit_rate = 2700000000;
		else
			tmp_lane_para->bit_rate = 0;
	}

	/* lane count */
	regval = GET_BITS(6, 2, reg_val);
	if (regval == 0)
		tmp_lane_para->lane_cnt = 1;
	else if (regval == 1)
		tmp_lane_para->lane_cnt = 2;
	else if (regval == 2)
		tmp_lane_para->lane_cnt = 4;
	else
		tmp_lane_para->lane_cnt = 0;

	/*sw*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
	regval = GET_BITS(0, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane0_sw = 0;
	if (regval == 2)
		tmp_lane_para->lane0_sw = 1;
	if (regval == 4)
		tmp_lane_para->lane0_sw = 2;
	if (regval == 6)
		tmp_lane_para->lane0_sw = 3;

	regval = GET_BITS(4, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane1_sw = 0;
	if (regval == 2)
		tmp_lane_para->lane1_sw = 1;
	if (regval == 4)
		tmp_lane_para->lane1_sw = 2;
	if (regval == 6)
		tmp_lane_para->lane1_sw = 3;

	reg_val = readl(edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
	regval = GET_BITS(24, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane2_sw = 0;
	if (regval == 2)
		tmp_lane_para->lane2_sw = 1;
	if (regval == 4)
		tmp_lane_para->lane2_sw = 2;
	if (regval == 6)
		tmp_lane_para->lane2_sw = 3;

	regval = GET_BITS(28, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane3_sw = 0;
	if (regval == 2)
		tmp_lane_para->lane3_sw = 1;
	if (regval == 4)
		tmp_lane_para->lane3_sw = 2;
	if (regval == 6)
		tmp_lane_para->lane3_sw = 3;


	/*pre*/
	reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
	regval = GET_BITS(24, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane0_pre = 0;
	if (regval == 1)
		tmp_lane_para->lane0_pre = 1;
	if (regval == 2)
		tmp_lane_para->lane0_pre = 2;
	if (regval == 3)
		tmp_lane_para->lane0_pre = 3;

	regval = GET_BITS(28, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane1_pre = 0;
	if (regval == 1)
		tmp_lane_para->lane1_pre = 1;
	if (regval == 2)
		tmp_lane_para->lane1_pre = 2;
	if (regval == 3)
		tmp_lane_para->lane1_pre = 3;

	regval = GET_BITS(16, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane2_pre = 0;
	if (regval == 1)
		tmp_lane_para->lane2_pre = 1;
	if (regval == 2)
		tmp_lane_para->lane2_pre = 2;
	if (regval == 3)
		tmp_lane_para->lane2_pre = 3;

	regval = GET_BITS(20, 4, reg_val);
	if (regval == 0)
		tmp_lane_para->lane3_pre = 0;
	if (regval == 1)
		tmp_lane_para->lane3_pre = 1;
	if (regval == 2)
		tmp_lane_para->lane3_pre = 2;
	if (regval == 3)
		tmp_lane_para->lane3_pre = 3;

	return RET_OK;
}

s32 edp_hal_get_tu_size(u32 sel)
{
	return LS_PER_TU;
}

s32 edp_hal_get_valid_symbol_per_tu(u32 sel)
{
	u32 reg_val;
	u32 regval;
	u32 valid_symbol;

	reg_val = readl(edp_base[sel] + REG_EDP_FRAME_UNIT);
	regval = GET_BITS(0, 7, reg_val);
	valid_symbol = regval * 10;

	regval = GET_BITS(16, 4, reg_val);
	valid_symbol += regval;

	return valid_symbol;
}

bool edp_hal_audio_is_enabled(u32 sel)
{
	u32 reg_val;
	u32 regval0;
	u32 regval1;
	u32 regval2;
	u32 regval3;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_HBLANK_EN);
	regval0 = GET_BITS(0, 1, reg_val);
	regval1 = GET_BITS(1, 1, reg_val);

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO_VBLANK_EN);
	regval2 = GET_BITS(0, 1, reg_val);
	regval3 = GET_BITS(1, 1, reg_val);

	if (regval0 && regval1 && regval2 && regval3)
		return true;

	return false;
}

s32 edp_hal_get_audio_if(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = GET_BITS(0, 1, reg_val);

	return reg_val;
}

s32 edp_hal_audio_is_mute(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = GET_BITS(15, 1, reg_val);

	return reg_val;
}

s32 edp_hal_get_audio_chn_cnt(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = GET_BITS(12, 3, reg_val);

	if (reg_val == 0)
		return 1;
	else if (reg_val == 1)
		return 2;
	else
		return 8;
}

s32 edp_hal_get_audio_date_width(u32 sel)
{
	u32 reg_val;

	reg_val = readl(edp_base[sel] + REG_EDP_AUDIO);
	reg_val = GET_BITS(5, 5, reg_val);

	if (reg_val == 0x10)
		return 16;
	else if (reg_val == 0x14)
		return 20;
	else
		return 24;
}

s32 edp_hal_read_dpcd(u32 sel, char *dpcd_rx_buf)
{
	s32 i, blk, ret = 0;

	/*link configuration*/
	blk = 16;
	/* read 16 Byte dp sink capability */
	for (i = 0; i < 256 / blk; i++) {
		ret = edp_aux_read(sel, 0x0000 + i * blk, blk, (char *)(dpcd_rx_buf) + (i * blk));
		if (ret < 0)
			return ret;
	}

	switch (dpcd_rx_buf[0x0e]) {
	case 0x00:
		/*Link Status/Adjust Request read interval during CR*/
		/*phase --- 100us*/
		training_interval_CR = 100;
		/*Link Status/Adjust Request read interval during EQ*/
		/*phase --- 400us*/
		training_interval_EQ = 400;

		break;
	case 0x01:
		training_interval_CR = 4000;
		training_interval_EQ = 4000;
		break;
	case 0x02:
		training_interval_CR = 8000;
		training_interval_EQ = 8000;
		break;
	case 0x03:
		training_interval_CR = 12000;
		training_interval_EQ = 12000;
		break;
	case 0x04:
		training_interval_CR = 16000;
		training_interval_EQ = 16000;
		break;
	default:
		training_interval_CR = 100;
		training_interval_EQ = 400;
	}

	return ret;
}

s32 edp_hal_get_cur_line(u32 sel)
{
	return 0;
}

s32 edp_hal_get_start_dly(u32 sel)
{
	return 0;
}

void edp_hal_show_builtin_patten(u32 sel, u32 pattern)
{
}


void edp_hal_clean_hpd_status(u32 sel)
{
	writel((1 << 1), edp_base[sel] + REG_EDP_HPD_PLUG);
}


s32 edp_hal_link_start(u32 sel)
{
	edp_video_stream_enable(sel);

	return RET_OK;
}

bool edp_hal_support_tps3(u32 sel)
{
	return false;
}

bool edp_hal_support_fast_train(u32 sel)
{
	return false;
}

void edp_hal_set_lane_swing_voltage(u32 sel, u32 lane_num, u32 sw)
{
	u32 reg_val;

	if (lane_num == 0) {
		/* lane0 swing voltage level*/
		reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
		if (sw == 0)
			reg_val = SET_BITS(0, 4, reg_val, 0);
		else if (sw == 1)
			reg_val = SET_BITS(0, 4, reg_val, 2);
		else if (sw == 2)
			reg_val = SET_BITS(0, 4, reg_val, 4);
		else if (sw == 3)
			reg_val = SET_BITS(0, 4, reg_val, 6);
		else
			reg_val = SET_BITS(0, 4, reg_val, 0);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);
	} else if (lane_num == 1) {
		/* lane1 swing voltage level*/
		reg_val = readl(edp_base[sel] + REG_EDP_TX_MAINSEL);
		if (sw == 0)
			reg_val = SET_BITS(4, 4, reg_val, 0);
		else if (sw == 1)
			reg_val = SET_BITS(4, 4, reg_val, 2);
		else if (sw == 2)
			reg_val = SET_BITS(4, 4, reg_val, 4);
		else if (sw == 3)
			reg_val = SET_BITS(4, 4, reg_val, 6);
		else
			reg_val = SET_BITS(4, 4, reg_val, 0);

		writel(reg_val, edp_base[sel] + REG_EDP_TX_MAINSEL);
	} else if (lane_num == 2) {
		/* lane2 swing voltage level*/
		reg_val = readl(edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
		if (sw == 0)
			reg_val = SET_BITS(24, 4, reg_val, 0);
		else if (sw == 1)
			reg_val = SET_BITS(24, 4, reg_val, 2);
		else if (sw == 2)
			reg_val = SET_BITS(24, 4, reg_val, 4);
		else if (sw == 3)
			reg_val = SET_BITS(24, 4, reg_val, 6);
		else
			reg_val = SET_BITS(24, 4, reg_val, 0);
		writel(reg_val, edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
	} else if (lane_num == 3) {
		/* lane3 swing voltage level*/
		reg_val = readl(edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
		if (sw == 0)
			reg_val = SET_BITS(28, 4, reg_val, 0);
		else if (sw == 1)
			reg_val = SET_BITS(28, 4, reg_val, 2);
		else if (sw == 2)
			reg_val = SET_BITS(28, 4, reg_val, 4);
		else if (sw == 3)
			reg_val = SET_BITS(28, 4, reg_val, 6);
		else
			reg_val = SET_BITS(28, 4, reg_val, 0);
		writel(reg_val, edp_base[sel] + REG_EDP_TX32_ISEL_DRV);
	} else
		EDP_WRN("%s: lane number is not support!\n", __func__);
}


void edp_hal_sel_lane_pre_emphasis(u32 sel, u32 lane_num, u32 pre)
{
	u32 reg_val = 0;

	if (lane_num == 0) {
		/* lane0 pre emphasis level */
		reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
		if (pre == 0)
			reg_val = SET_BITS(24, 4, reg_val, 0);
		else if (pre == 1)
			reg_val = SET_BITS(24, 4, reg_val, 1);
		else if (pre == 2)
			reg_val = SET_BITS(24, 4, reg_val, 2);
		else if (pre == 3)
			reg_val = SET_BITS(24, 4, reg_val, 3);
		else
			reg_val = SET_BITS(24, 4, reg_val, 0);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);
	} else if (lane_num == 1) {
		/* lane1 pre emphasis level */
		reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
		if (pre == 0)
			reg_val = SET_BITS(28, 4, reg_val, 0);
		else if (pre == 1)
			reg_val = SET_BITS(28, 4, reg_val, 1);
		else if (pre == 2)
			reg_val = SET_BITS(28, 4, reg_val, 2);
		else if (pre == 3)
			reg_val = SET_BITS(28, 4, reg_val, 3);
		else
			reg_val = SET_BITS(28, 4, reg_val, 0);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);
	} else if (lane_num == 2) {
		/* lane2 pre emphasis level */
		reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
		if (pre == 0)
			reg_val = SET_BITS(16, 4, reg_val, 0);
		else if (pre == 1)
			reg_val = SET_BITS(16, 4, reg_val, 1);
		else if (pre == 2)
			reg_val = SET_BITS(16, 4, reg_val, 2);
		else if (pre == 3)
			reg_val = SET_BITS(16, 4, reg_val, 3);
		else
			reg_val = SET_BITS(16, 4, reg_val, 0);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);
	} else if (lane_num == 3) {
		/* lane2 pre emphasis level */
		reg_val = readl(edp_base[sel] + REG_EDP_TX_POSTSEL);
		if (pre == 0)
			reg_val = SET_BITS(20, 4, reg_val, 0);
		else if (pre == 1)
			reg_val = SET_BITS(20, 4, reg_val, 1);
		else if (pre == 2)
			reg_val = SET_BITS(20, 4, reg_val, 2);
		else if (pre == 3)
			reg_val = SET_BITS(20, 4, reg_val, 3);
		else
			reg_val = SET_BITS(20, 4, reg_val, 0);
		writel(reg_val, edp_base[sel] + REG_EDP_TX_POSTSEL);
	} else
		EDP_WRN("%s: lane number is not support!\n", __func__);
}

void edp_hal_set_lane_rate(u32 sel, u64 bit_rate)
{
	u32 reg_val;

	/*config lane bit rate*/
	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	switch (bit_rate) {
	case BIT_RATE_1G62:
		reg_val = SET_BITS(26, 3, reg_val, 0x0);
		reg_val = SET_BITS(4, 2, reg_val, 0x0);
		break;
	case BIT_RATE_2G7:
	default:
		reg_val = SET_BITS(26, 3, reg_val, 0x0);
		reg_val = SET_BITS(4, 2, reg_val, 0x1);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);
}

void edp_hal_set_lane_cnt(u32 sel, u32 lane_cnt)
{
	u32 reg_val;

	if ((lane_cnt < 0) || (lane_cnt > 4)) {
		EDP_WRN("unsupport lane number!\n");
	}

	/*config lane number*/
	reg_val = readl(edp_base[sel] + REG_EDP_CAPACITY);
	switch (lane_cnt) {
	case 0:
	case 3:
		EDP_WRN("edp lane number can not be configed to 0/3!\n");
	case 1:
		reg_val = SET_BITS(6, 2, reg_val, 0x0);
		reg_val = SET_BITS(8, 4, reg_val, 0x01);
		break;
	case 2:
		reg_val = SET_BITS(6, 2, reg_val, 0x1);
		reg_val = SET_BITS(8, 4, reg_val, 0x3);
		break;
	case 4:
		reg_val = SET_BITS(6, 2, reg_val, 0x2);
		reg_val = SET_BITS(8, 4, reg_val, 0xf);
		break;
	}
	writel(reg_val, edp_base[sel] + REG_EDP_CAPACITY);
}

