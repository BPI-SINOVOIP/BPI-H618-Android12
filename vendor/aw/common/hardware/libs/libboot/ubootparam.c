/*
 * Sunxi Soc uboot env update utils
 * Base on u-boot/tools/env
 *
 * Copyright (C) 2015-2018 AllwinnerTech, Inc.
 *
 * Contacts:
 * Zeng.Yajian <zengyajian@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include "libboot.h"

static void
usage(FILE *out, const char *name) {
	fprintf(out, "Usage: %s [-h] [-p] [-w name value] [-r name]\n\n", name);
	fprintf(out,
		"       -h, --help  Show this help message\n"
		"       -p, --print Print the whole uboot env\n"
		"       -w, --write Write env 'name' with 'value'\n");
}

const struct option longopts[] = {
	{"help",  no_argument,       0, 'h'},
	{"print", no_argument,       0, 'p'},
	{"write", required_argument, 0, 'w'},
	{0, 0, 0, 0}
};

int print_display_param(void);

int main (int argc, char **argv)
{
	int ch;
	int help, argout, action;
	char *name = 0;

	action = help = argout = 0;
	while (!argout &&
		(ch = getopt_long(argc, argv, "hpw:", longopts, 0)) != -1) {
		switch (ch) {
		case 'p':
			print_display_param();
			argout = 1;
			break;
		case 'w':
			action = 1;
			name = optarg;
			break;
		case 'h':
		default :
			help = argout = 1;
		}
	}
	if (help || argc == 1) {
		usage(stdout, argv[0]);
		return EXIT_SUCCESS;
	}
	argc -= optind;

	if (action == 1) {
		if (argc != 1) goto _error;
		char *value = (argv+optind)[0];

		libboot_sync_display_param_keyvalue(name, value);
	}

	return 0;

_error:
	usage(stderr, argv[0]);
	return EXIT_FAILURE;
}

static char *__format_string(char *str)
{
	char *p = str;
	while (*p != 0) {
		if (*p == '\n') *p = ' ';
		p++;
	}
	return str;
}

int print_display_param(void)
{
	struct user_display_param param;

	memset(&param, 0, sizeof(param));
	if (libboot_read_display_param(&param) != 0)
		fprintf(stderr, "libboot read display params failed\n");

	fprintf(stdout, "resolution : %s\n", __format_string(param.resolution));
	fprintf(stdout, "margin     : %s\n", __format_string(param.margin));
	fprintf(stdout, "vendorid   : %s\n", __format_string(param.vendorid));

	fprintf(stdout, "format     : %08x\n", param.format);
	fprintf(stdout, "color depth: %08x\n", param.depth);
	fprintf(stdout, "color space: %08x\n", param.color_space);
	fprintf(stdout, "eotf       : %08x\n", param.eotf);
	return 0;
}
