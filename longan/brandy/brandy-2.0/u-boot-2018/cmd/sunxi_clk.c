/*
 * Copyright (C) 2016 Allwinnertech
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Adjustable factor-based clock implementation
 */
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <fdt_support.h>
#include <linux/compat.h>
#include <clk/clk.h>

static int do_sunxi_clk_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int i = 0;
	struct clk *clk;
	int ret;
	struct clk *parent_clk;
	unsigned long rate;
	char *test_clk[1024] = {"hosc", "pll_periph0_2x", "pll_periph1_2x", "pll_periph1_800m", "pll_periph1_600m", "pll_periph0_300m", "sdmmc0_mod", "sdmmc2_mod"};

	for (i = 0; i < 9; i++) {
		/* 获得时钟 */
		clk = clk_get(NULL, test_clk[i]);
		if (!clk) {
			printf("error to get clk: %s\n", test_clk[i]);
		} else {
			printf("success get clk: %s\n", test_clk[i]);
		}

		ret = clk_prepare_enable(clk);
		if (ret)
			printf("enable clk: %s failed!\n", clk->name);
		else
			printf("enable clk: %s \n", clk->name);

		/* 获得父时钟 */
		parent_clk = clk_get_parent(clk);
		if (!parent_clk || IS_ERR(parent_clk)) {
			printf("error to get clk: %s parent_name: %s\n", test_clk[i], parent_clk->name);
		} else {
			printf("success to get clk: %s parent_name: %s\n", test_clk[i], parent_clk->name);
		}

		/* 获得当前频率 */
		rate = clk_get_rate(clk);
		printf("success to get clk: %s parent_name: %s\n rate: %ld\n", test_clk[i], parent_clk->name, rate);
	}

	return 0;
}

U_BOOT_CMD(
		sunxi_clk,	3,	1,	do_sunxi_clk_test,
		"do clk test",
		"sunxi_clk test_cmd"
	  );
