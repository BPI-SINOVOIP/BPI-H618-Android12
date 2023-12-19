// SPDX-License-Identifier: MIT
/*
 * The 'fsverity sign' command
 *
 * Copyright 2018 Google LLC
 *
 * Use of this source code is governed by an MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT.
 */

#include "fsverity.h"

#include <fcntl.h>
#include <getopt.h>

static bool write_signature(const char *filename, const u8 *sig, u32 sig_size)
{
	struct filedes file;
	bool ok;

	if (!open_file(&file, filename, O_WRONLY|O_CREAT|O_TRUNC, 0644))
		return false;
	ok = full_write(&file, sig, sig_size);
	ok &= filedes_close(&file);
	return ok;
}

static const struct option longopts[] = {
	{"hash-alg",	required_argument, NULL, OPT_HASH_ALG},
	{"block-size",	required_argument, NULL, OPT_BLOCK_SIZE},
	{"salt",	required_argument, NULL, OPT_SALT},
	{"key",		required_argument, NULL, OPT_KEY},
	{"cert",	required_argument, NULL, OPT_CERT},
	{NULL, 0, NULL, 0}
};

/* Sign a file for fs-verity by computing its digest, then signing it. */
int fsverity_cmd_sign(const struct fsverity_command *cmd,
		      int argc, char *argv[])
{
	struct filedes file = { .fd = -1 };
	struct libfsverity_merkle_tree_params tree_params = { .version = 1 };
	struct libfsverity_signature_params sig_params = {};
	struct libfsverity_digest *digest = NULL;
	char digest_hex[FS_VERITY_MAX_DIGEST_SIZE * 2 + 1];
	u8 *sig = NULL;
	size_t sig_size;
	int status;
	int c;

	while ((c = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
		switch (c) {
		case OPT_HASH_ALG:
		case OPT_BLOCK_SIZE:
		case OPT_SALT:
			if (!parse_tree_param(c, optarg, &tree_params))
				goto out_usage;
			break;
		case OPT_KEY:
			if (sig_params.keyfile != NULL) {
				error_msg("--key can only be specified once");
				goto out_usage;
			}
			sig_params.keyfile = optarg;
			break;
		case OPT_CERT:
			if (sig_params.certfile != NULL) {
				error_msg("--cert can only be specified once");
				goto out_usage;
			}
			sig_params.certfile = optarg;
			break;
		default:
			goto out_usage;
		}
	}

	argv += optind;
	argc -= optind;

	if (argc != 2)
		goto out_usage;

	if (sig_params.keyfile == NULL) {
		error_msg("Missing --key argument");
		goto out_usage;
	}
	if (sig_params.certfile == NULL)
		sig_params.certfile = sig_params.keyfile;

	if (!open_file(&file, argv[0], O_RDONLY, 0))
		goto out_err;

	if (!get_file_size(&file, &tree_params.file_size))
		goto out_err;

	if (libfsverity_compute_digest(&file, read_callback,
				       &tree_params, &digest) != 0) {
		error_msg("failed to compute digest");
		goto out_err;
	}

	if (libfsverity_sign_digest(digest, &sig_params,
				    &sig, &sig_size) != 0) {
		error_msg("failed to sign digest");
		goto out_err;
	}

	if (!write_signature(argv[1], sig, sig_size))
		goto out_err;

	ASSERT(digest->digest_size <= FS_VERITY_MAX_DIGEST_SIZE);
	bin2hex(digest->digest, digest->digest_size, digest_hex);
	printf("Signed file '%s' (%s:%s)\n", argv[0],
	       libfsverity_get_hash_name(digest->digest_algorithm), digest_hex);
	status = 0;
out:
	filedes_close(&file);
	destroy_tree_params(&tree_params);
	free(digest);
	free(sig);
	return status;

out_err:
	status = 1;
	goto out;

out_usage:
	usage(cmd, stderr);
	status = 2;
	goto out;
}
