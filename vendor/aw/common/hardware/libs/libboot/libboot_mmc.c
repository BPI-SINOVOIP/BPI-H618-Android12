/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libboot.h"
#include "libmmc.h"
#include "sunxi_boot_api.h"
#include "private_uboot.h"
#include "private_boot0.h"
#include "private_toc.h"

#define LIBOOT_EMMC_DEBUG

static int copy_mmc_info(void *in, void *out, u32 length)
{

	if ((in == NULL) || (out == NULL) || (length < sizeof(struct boot_sdmmc_private_info_t))) {
		bb_debug("%s %d:wrong input arg\n", __func__, __LINE__);
		return -1;
	}

	/*
	 *----- normal
	 *	offset 0~127: boot0 struct _boot_sdcard_info_t;
	 *	offset 128~255: struct tune_sdly, timing
	 *	parameter for speed mode and frequency
	 *----- secure
	 *	offset 128 ~ (224=384-160):
	 *	struct tune_sdly, timing
	 *	rameter for speed mode and
	 *	frequency
	 *	sizeof(priv_info)  is about 60 bytes.
	 *
	 * */

	memcpy((void *)((unsigned char*)out + SDMMC_PRIV_INFO_ADDR_OFFSET),
		(void *)((unsigned char*)in + SDMMC_PRIV_INFO_ADDR_OFFSET),
		sizeof(struct boot_sdmmc_private_info_t));

#if defined(LIBOOT_EMMC_DEBUG)
	{
		u32 i, *p;
		printf("out buffer\n");
		p = (u32 *)((unsigned char *)out + SDMMC_PRIV_INFO_ADDR_OFFSET);
		for (i = 0; i < sizeof(struct boot_sdmmc_private_info_t)/4; i++)
		printf("%d %x\n", i, p[i]);
	}
	{
		u32 i, *p;
		printf("int buffer\n");
		p = (u32 *)((unsigned char *)in + SDMMC_PRIV_INFO_ADDR_OFFSET);
		for (i = 0; i < sizeof(struct boot_sdmmc_private_info_t)/4; i++)
		printf("%d %x\n", i, p[i]);
	}
#endif
	return 0;
}

static int read_mmc_parameter(void *buffer, void *param, void *item_name)
{
	struct sbrom_toc0_config *toc0_config = NULL;
	boot0_file_head_t *boot0_file = NULL;
	char *old_param = NULL;
	toc0_private_head_t * toc0_head = NULL;
	if (!buffer || !param)
		return -1;

	if (check_soc_is_secure() == 0) {
		boot0_file = (boot0_file_head_t *)buffer;
		old_param = (char *)(boot0_file->prvt_head.storage_data);
		copy_mmc_info(old_param, param, 256);
	} else {
		toc0_head = (toc0_private_head_t *)buffer;
		if (toc0_head->items_nr == 2) {
			toc0_config = (struct sbrom_toc0_config *)((char *)buffer + 0x80);
		} else {
			toc0_config = (struct sbrom_toc0_config *)((char *)buffer + 0xa0);
		}
		old_param = (char *)((char *)toc0_config->storage_data + 160);
		copy_mmc_info(old_param, param, 384 - 160);
	}
	return 0;
}

static int sync_mmc_parameter(void *buffer, void *param, void *item_name)
{
	toc0_private_head_t *toc0_head = NULL;
	struct sbrom_toc0_config *toc0_config = NULL;
	boot0_file_head_t *boot0_file = NULL;
	char *old_param = NULL;

	if (!buffer || !param)
		return -1;

	if (check_soc_is_secure() == 0) {
		boot0_file = (boot0_file_head_t *)buffer;
		old_param = (char *)(boot0_file->prvt_head.storage_data);

		/*modify mmc data, mabye this is restore*/
		copy_mmc_info(param, old_param, STORAGE_BUFFER_SIZE);

		boot0_file->boot_head.check_sum = generate_check_sum(buffer, boot0_file->boot_head.length, boot0_file->boot_head.check_sum);
	} else {
		toc0_head = (toc0_private_head_t *)buffer;
		if (toc0_head->items_nr == 2) {
			toc0_config = (struct sbrom_toc0_config *)((char *)buffer + 0x80);
		} else {
			toc0_config = (struct sbrom_toc0_config *)((char *)buffer + 0xa0);
		}
		old_param = (char *)((char *)toc0_config->storage_data + 160);
		/*modify mmc data, mabye this is restore*/
		copy_mmc_info(param, old_param, 384-160);
		toc0_head->check_sum = generate_check_sum(buffer, toc0_head->length, toc0_head->check_sum);
	}

	return 0;
}

//FIX ME
static int burn_mmc_parameter(void *cookie, void *in, void *item)
{
	int ret = -1;
	BufferExtractCookie *out_cookie = NULL;
	BufferExtractCookie *in_cookie = NULL;

	if (!cookie || !in || !item)
		return -1;

	out_cookie = (BufferExtractCookie *)cookie;
	in_cookie = (BufferExtractCookie *)in;

	if ((out_cookie->buffer == NULL) || (in_cookie->buffer == NULL))
		return -1;

	if ((in_cookie->len > 0) && (in_cookie->len <= out_cookie->len)) {
		memcpy(out_cookie->buffer, in_cookie->buffer, in_cookie->len);
		ret = sync_mmc_parameter(out_cookie->buffer, item, NULL);
	}
	return ret;
}

int libboot_read_mmc_param(void *out)
{
	return libboot_read_boot(read_mmc_parameter, out, BOOT0, NULL);
}

int libboot_burn_mmc_para(void *in, void *param)
{
	return libboot_burn_boot(burn_mmc_parameter, in, BOOT0, param);
}
