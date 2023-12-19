// SPDX-License-Identifier: MIT
/*
 * fs-verity userspace tool
 *
 * Copyright 2018 Google LLC
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "fsverity.h"

#include <limits.h>

static const struct fsverity_command {
	const char *name;
	int (*func)(const struct fsverity_command *cmd, int argc, char *argv[]);
	const char *short_desc;
	const char *usage_str;
} fsverity_commands[] = {
	{
		.name = "digest",
		.func = fsverity_cmd_digest,
		.short_desc =
"Compute the fs-verity digest of the given file(s), for offline signing",
		.usage_str =
"    fsverity digest FILE...\n"
"               [--hash-alg=HASH_ALG] [--block-size=BLOCK_SIZE] [--salt=SALT]\n"
"               [--compact] [--for-builtin-sig]\n"
#ifndef _WIN32
	}, {
		.name = "enable",
		.func = fsverity_cmd_enable,
		.short_desc = "Enable fs-verity on a file",
		.usage_str =
"    fsverity enable FILE\n"
"               [--hash-alg=HASH_ALG] [--block-size=BLOCK_SIZE] [--salt=SALT]\n"
"               [--signature=SIGFILE]\n"
	}, {
		.name = "measure",
		.func = fsverity_cmd_measure,
		.short_desc =
"Display the fs-verity digest of the given verity file(s)",
		.usage_str =
"    fsverity measure FILE...\n"
#endif /* !_WIN32 */
	}, {
		.name = "sign",
		.func = fsverity_cmd_sign,
		.short_desc = "Sign a file for fs-verity",
		.usage_str =
"    fsverity sign FILE OUT_SIGFILE --key=KEYFILE\n"
"               [--hash-alg=HASH_ALG] [--block-size=BLOCK_SIZE] [--salt=SALT]\n"
"               [--cert=CERTFILE]\n"
	}
};

static void show_all_hash_algs(FILE *fp)
{
	u32 alg_num = 1;
	const char *name;

	fprintf(fp, "Available hash algorithms:");
	while ((name = libfsverity_get_hash_name(alg_num++)) != NULL)
		fprintf(fp, " %s", name);
	putc('\n', fp);
}

static void usage_all(FILE *fp)
{
	int i;

	fputs("Usage:\n", fp);
	for (i = 0; i < ARRAY_SIZE(fsverity_commands); i++)
		fprintf(fp, "  %s:\n%s\n", fsverity_commands[i].short_desc,
			fsverity_commands[i].usage_str);
	fputs(
"  Standard options:\n"
"    fsverity --help\n"
"    fsverity --version\n"
"\n", fp);
	show_all_hash_algs(fp);
}

static void usage_cmd(const struct fsverity_command *cmd, FILE *fp)
{
	fprintf(fp, "Usage:\n%s", cmd->usage_str);
}

void usage(const struct fsverity_command *cmd, FILE *fp)
{
	if (cmd)
		usage_cmd(cmd, fp);
	else
		usage_all(fp);
}

static void show_version(void)
{
	printf("fsverity v%d.%d\n", FSVERITY_UTILS_MAJOR_VERSION,
	       FSVERITY_UTILS_MINOR_VERSION);
}

static void handle_common_options(int argc, char *argv[],
				  const struct fsverity_command *cmd)
{
	int i;

	for (i = 1; i < argc; i++) {
		const char *arg = argv[i];

		if (*arg++ != '-')
			continue;
		if (*arg++ != '-')
			continue;
		if (!strcmp(arg, "help")) {
			usage(cmd, stdout);
			exit(0);
		} else if (!strcmp(arg, "version")) {
			show_version();
			exit(0);
		} else if (!*arg) /* reached "--", no more options */
			return;
	}
}

static const struct fsverity_command *find_command(const char *name)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fsverity_commands); i++)
		if (!strcmp(name, fsverity_commands[i].name))
			return &fsverity_commands[i];
	return NULL;
}

static bool parse_hash_alg_option(const char *arg, u32 *alg_ptr)
{
	char *end;
	unsigned long n = strtoul(arg, &end, 10);

	if (*alg_ptr != 0) {
		error_msg("--hash-alg can only be specified once");
		return false;
	}

	/* Specified by number? */
	if (n > 0 && n < INT32_MAX && *end == '\0') {
		*alg_ptr = n;
		return true;
	}

	/* Specified by name? */
	*alg_ptr = libfsverity_find_hash_alg_by_name(arg);
	if (*alg_ptr)
		return true;
	error_msg("unknown hash algorithm: '%s'", arg);
	show_all_hash_algs(stderr);
	return false;
}

static bool parse_block_size_option(const char *arg, u32 *size_ptr)
{
	char *end;
	unsigned long n = strtoul(arg, &end, 10);

	if (*size_ptr != 0) {
		error_msg("--block-size can only be specified once");
		return false;
	}

	if (n <= 0 || n >= INT_MAX || !is_power_of_2(n) || *end != '\0') {
		error_msg("Invalid block size: %s.  Must be power of 2", arg);
		return false;
	}
	*size_ptr = n;
	return true;
}

static bool parse_salt_option(const char *arg, u8 **salt_ptr,
			      u32 *salt_size_ptr)
{
	if (*salt_ptr != NULL) {
		error_msg("--salt can only be specified once");
		return false;
	}
	*salt_size_ptr = strlen(arg) / 2;
	*salt_ptr = xmalloc(*salt_size_ptr);
	if (!hex2bin(arg, *salt_ptr, *salt_size_ptr)) {
		error_msg("salt is not a valid hex string");
		return false;
	}
	return true;
}

bool parse_tree_param(int opt_char, const char *arg,
		      struct libfsverity_merkle_tree_params *params)
{
	switch (opt_char) {
	case OPT_HASH_ALG:
		return parse_hash_alg_option(arg, &params->hash_algorithm);
	case OPT_BLOCK_SIZE:
		return parse_block_size_option(arg, &params->block_size);
	case OPT_SALT:
		return parse_salt_option(arg, (u8 **)&params->salt,
					 &params->salt_size);
	default:
		ASSERT(0);
	}
}

void destroy_tree_params(struct libfsverity_merkle_tree_params *params)
{
	free((u8 *)params->salt);
	memset(params, 0, sizeof(*params));
}

int main(int argc, char *argv[])
{
	const struct fsverity_command *cmd;

	install_libfsverity_error_handler();

	if (argc < 2) {
		error_msg("no command specified");
		usage_all(stderr);
		return 2;
	}

	cmd = find_command(argv[1]);

	handle_common_options(argc, argv, cmd);

	if (!cmd) {
		error_msg("unrecognized command: '%s'", argv[1]);
		usage_all(stderr);
		return 2;
	}
	return cmd->func(cmd, argc - 1, argv + 1);
}
