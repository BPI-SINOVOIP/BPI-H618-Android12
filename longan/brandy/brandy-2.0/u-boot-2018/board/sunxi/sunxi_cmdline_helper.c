/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2022-2024
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * ouyangkun <ouyangkun@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <command.h>
#include <linux/list.h>
#include <string.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <sunxi_board.h>

int sunxi_cmdline_parse(const char *string, struct key_value_set *buffer,
			size_t buffer_count)
{
	const char *p			  = string;
	int i				  = 0;
	enum { KEY, VALUE } searching_for = KEY;
	buffer[i].key			  = NULL;
	buffer[i].value			  = NULL;
	buffer[i].key_len		  = 0;
	buffer[i].value_len		  = 0;
	/* loop until last char*/
	while (*p != '\0') {
		switch (searching_for) {
		case KEY:
			if ((*p == '\0') || (*p == ' ') || (*p == 0xa)) {
				//separator, skip
				if (buffer[i].key != NULL) {
					//separator before '=',next key
					i++;
					if (i < buffer_count) {
						buffer[i].key	    = NULL;
						buffer[i].value	    = NULL;
						buffer[i].key_len   = 0;
						buffer[i].value_len = 0;
					}
				}
			} else if (*p == '=') {
				if (buffer[i].key != NULL) {
					//found '=',start seraching value
					searching_for = VALUE;
				} else {
					//just in case string start with '='
				}
			} else {
				if (i >= buffer_count) {
					//no more space in buffer for next key value set
					//before parsing whole string
					return -ENOMEM;
				}
				if (buffer[i].key == NULL) {
					//first char of key
					buffer[i].key = p;
				}
				buffer[i].key_len++;
			}
			break;
		case VALUE:
			if ((*p == '\0') || (*p == ' ') || (*p == 0xa)) {
				//separator, value end, start searching key
				searching_for = KEY;
				i++;
				if (i < buffer_count) {
					buffer[i].key	    = NULL;
					buffer[i].value	    = NULL;
					buffer[i].key_len   = 0;
					buffer[i].value_len = 0;
				}
			} else {
				if (buffer[i].value == NULL) {
					//first char of value
					buffer[i].value = p;
				}
				buffer[i].value_len++;
			}
			break;
		}
		p++;
	}
	return i;
}

static int do_test(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	const char *test =
		"holder=${holder} rcutree.rcu_idle_gp_delay= earlycon=${earlycon} clk_ignore_unused initcall_debug=${initcall_debug} console=${console} loglevel=${loglevel} root=${mmc_root} init=${init} cma=${cma} snum=${snum} mac_addr=${mac} wifi_mac=${wifi_mac} bt_mac=${bt_mac} specialstr=${specialstr} gpt=1 androidboot.force_normal_boot=${force_normal_boot} androidboot.slot_suffix=${slot_suffix}";
	struct key_value_set parsed[32];
	char print_fmt[17];
	int i, count;
	memset(parsed, 0, sizeof(parsed));
	if (argc == 2)
		test = argv[1];

	count = sunxi_cmdline_parse(test, parsed, ARRAY_SIZE(parsed));
	printf("count %d\n", count);
	printf("for string-->: %s\n", test);
	printf("parsed:\n");
	for (i = 0; i < count; i++) {
		snprintf(print_fmt, sizeof(print_fmt), "%%.%ds : %%.%ds",
			 parsed[i].key_len, parsed[i].value_len);
		printf("%s", print_fmt);
		printf(print_fmt, parsed[i].key, parsed[i].value);
		printf("\n");
	}
	return 0;
}
U_BOOT_CMD(sunxi_cmd_parser, 6, 1, do_test, "sunxi cmdline parser test", "");