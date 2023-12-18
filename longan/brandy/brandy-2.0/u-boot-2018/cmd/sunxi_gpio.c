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

#include <sys_config.h>
extern int fdt_set_all_pin(const char *node_path, const char *pinctrl_name);
static int do_sunxi_gpio_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	int ret = -1;

	ret = fdt_set_all_pin("/soc/lcd1", "pinctrl-0");
	if (ret != 0)
		printf("%s, fdt_set_all_pin, ret=%d\n", __func__, ret);

	printf("sunxi gpio test done\n");

	return ret;
 }
U_BOOT_CMD(
	sunxi_gpio,	3,	1,	do_sunxi_gpio_test,
	"do gpio test",
	"sunxi_gpio test_cmd"
);
