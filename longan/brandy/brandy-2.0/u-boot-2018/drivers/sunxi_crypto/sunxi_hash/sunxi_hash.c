/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <hash.h>
#include <u-boot/sha256.h>

int sunxi_sha_calc_with_software(char *algo_name, u8 *dst_addr, u32 dst_len, u8 *src_addr, u32 src_len)
{
	struct hash_algo *algo;

	pr_debug("%s start\n", __func__);
	if (hash_lookup_algo(algo_name, &algo)) {
		pr_err("Unknown hash algorithm '%s'\n", algo_name);
		return CMD_RET_USAGE;
	}

	if (algo->digest_size > HASH_MAX_DIGEST_SIZE) {
		pr_err("HASH_MAX_DIGEST_SIZE exceeded\n");
		return 1;
	}

	algo->hash_func_ws(src_addr, src_len, dst_addr, algo->chunk_size);
	pr_debug("%s end\n", __func__);
	return 0;
}
