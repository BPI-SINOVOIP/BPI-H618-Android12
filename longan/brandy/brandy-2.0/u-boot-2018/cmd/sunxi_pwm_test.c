// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <command.h>
#include <pwm.h>

int do_sunxi_pwm_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	u32 channel, duty, period;

	char pwm_str[30];
	if (argc < 3) {
		printf("pwm parameters error\n");
		return -1;
	} else {
		channel = simple_strtoul(argv[1], NULL, 10);
		duty = simple_strtoul(argv[2], NULL, 10);
		period = simple_strtoul(argv[3], NULL, 10);
	}
	printf("pwm test start\n");
	snprintf(pwm_str, sizeof(pwm_str), "sunxi_pwm%d", channel);
	pwm_request(channel, pwm_str);
	pwm_config(channel, duty, period);
	pwm_enable(channel);
	printf("Use an oscilloscope to check whether the pwm waveform is consistent with the setting\n");

	return 0;
}
U_BOOT_CMD(sunxi_pwm, CONFIG_SYS_MAXARGS, 1, do_sunxi_pwm_test, "do pwm test",  "channel duty period");
