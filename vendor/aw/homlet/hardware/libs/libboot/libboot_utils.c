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
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "type_def.h"
#include "private_boot0.h"
#include "private_toc.h"
#include "private_uboot.h"

#include "sunxi_boot_api.h"

#define LIBBOOT_VERSION		(1)

int get_libboot_version(void)
{
	return LIBBOOT_VERSION;
}


u32 randto1k(u32 num)
{
	if (num % 1024)
		return ((num + 1023) / 1024 *1024);
	else
		return num;
}

int read_file_to_cookie(const char *path, BufferExtractCookie *cookie)
{
	struct stat statbuff;
	FILE *fp = NULL;

	if (cookie == NULL)
		return -1;

	if (stat(path, &statbuff) < 0) {
		bb_debug("%s, get file %s stat failed\n", __func__, path);
		return -1;
	}

	memset(cookie->buffer, 0, cookie->len);

	cookie->len = statbuff.st_size;
	fp = fopen(path, "r");
	if (fp == NULL) {
		bb_debug("%s, open file %s failed\n", __func__, path);
		return -1;
	}
	fseek(fp, 0, SEEK_SET);
	if (fread(cookie->buffer, cookie->len, 1, fp) != 1) {
		bb_debug("%s, read file %s failed\n", __func__, path);
		fclose(fp);
		return -1;
	}
	cookie->len = randto1k(cookie->len);
	fclose(fp);
	return 0;
}

int get_cookie_of_file(const char *path, BufferExtractCookie *cookie)
{
	struct stat statbuff;
	char *buff = NULL;
	FILE *fp = NULL;

	if (cookie == NULL)
		return -1;

	if (stat(path, &statbuff) < 0) {
		bb_debug("%s, get file %s stat failed\n", __func__, path);
		return -1;
	}

	cookie->len = statbuff.st_size;
	buff = (char *)malloc(cookie->len);
	if (buff == NULL) {
		bb_debug("%s, cant malloc cookie\n", __func__);
		return -1;
	}

	fp = fopen(path, "r");
	if (fp == NULL) {
		bb_debug("%s, open file %s failed\n", __func__, path);
		free(buff);
		return -1;
	}

	if (!fread(buff, cookie->len, 1, fp)) {
		bb_debug("%s, read file %s failed\n", __func__, path);
		free(buff);
		fclose(fp);
		return -1;
	}

	cookie->buffer = buff;

	fclose(fp);
	return 0;
}

static int load_fip(void *buffer, parameter_callback cb, void *param, void *item_name)
{
	int ret = -1;
	unsigned int i;
	char *item_head_t = NULL;
	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;
	struct sbrom_toc1_item_info  *toc1_item = NULL;

	toc1_head = (struct sbrom_toc1_head_info  *)buffer;
	item_head = (struct sbrom_toc1_item_info  *)((char *)buffer + sizeof(struct sbrom_toc1_head_info));

#if defined(BOOT_DEBUG)
	bb_debug("*******************TOC1 Head Message*************************\n");
    bb_debug("Toc_name          = %s\n",   toc1_head->name);
    bb_debug("Toc_magic         = 0x%x\n", toc1_head->magic);
    bb_debug("Toc_add_sum       = 0x%x\n", toc1_head->add_sum);

    bb_debug("Toc_serial_num    = 0x%x\n", toc1_head->serial_num);
    bb_debug("Toc_status        = 0x%x\n", toc1_head->status);

    bb_debug("Toc_items_nr      = 0x%x\n", toc1_head->items_nr);
    bb_debug("Toc_valid_len     = 0x%x\n", toc1_head->valid_len);
    bb_debug("TOC_MAIN_END      = 0x%x\n", toc1_head->end);
    bb_debug("***************************************************************\n\n");
#endif

    if(toc1_head->magic != TOC_MAIN_INFO_MAGIC)
	{
		bb_debug("sunxi sprite: toc magic is error\n");
		return -1;
	}
	toc1_item = item_head;
	for(i = 0; i < toc1_head->items_nr; i++,toc1_item++) {

#if defined(BOOT_DEBUG)
		bb_debug("\n*******************TOC1 Item Message*************************\n");
        bb_debug("Entry_name        = %s\n",   toc1_item->name);
        bb_debug("Entry_data_offset = 0x%x\n", toc1_item->data_offset);
        bb_debug("Entry_data_len    = 0x%x\n", toc1_item->data_len);

        bb_debug("encrypt           = 0x%x\n", toc1_item->encrypt);
        bb_debug("Entry_type        = 0x%x\n", toc1_item->type);
        bb_debug("run_addr          = 0x%x\n", toc1_item->run_addr);
        bb_debug("index             = 0x%x\n", toc1_item->index);
        bb_debug("Entry_end         = 0x%x\n", toc1_item->end);
        bb_debug("***************************************************************\n\n");
#endif

        if(!strcmp(toc1_item->name, item_name))
        {
			item_head_t = ((char *)buffer + toc1_item->data_offset);

			ret = cb((char *)item_head_t, param);
			if(!ret)
				toc1_head->add_sum = generate_check_sum(buffer, toc1_head->valid_len, toc1_head->add_sum);
        }
	}

	return ret;
}

int libboot_read_item(parameter_callback cb, void *out, void *item_name)
{
	BufferExtractCookie cookie;
	int flash_type;
	int ret = -1;

	if (!cb || !out)
		return -EINVAL;

	flash_type = getFlashType();
	cookie.buffer = (char *)malloc(BOOT1_SIZE_BYTES);
	if(cookie.buffer == NULL) {
		bb_debug("%s cant malloc buffer\n", __func__);
		return -1;
	}

	cookie.len = BOOT1_SIZE_BYTES;
	if(!sunxi_read_uboot(&cookie, flash_type))
		ret = load_fip(cookie.buffer, cb, out, item_name);

	free(cookie.buffer);

	return ret;
}

int libboot_update_item(parameter_callback cb, void *in, void *item_name)
{
	BufferExtractCookie cookie;
	int flash_type;
	int needupdate = -1;

	if (!cb || !in)
		return -EINVAL;

	flash_type = getFlashType();
	cookie.buffer = (char *)malloc(BOOT1_SIZE_BYTES);
	if (cookie.buffer == NULL) {
		bb_debug("%s cant malloc buffer\n", __func__);
		return -1;
	}

	cookie.len = BOOT1_SIZE_BYTES;
	if (!sunxi_read_uboot(&cookie, flash_type))
		needupdate = load_fip(cookie.buffer, cb, in, item_name);

	if (needupdate == 0) {
		sunxi_write_uboot(&cookie, flash_type, 1);
	}

	free(cookie.buffer);
	return 0;
}

int libboot_burn_item(item_callback i_cb, void *in, void *item)
{
	BufferExtractCookie cookie;
	int flash_type = 0;
	int needupdate = -1;
	int ret = -1;

	if (!i_cb || !in || !item)
		return -EINVAL;

	flash_type = getFlashType();
	cookie.buffer = (char *)malloc(BOOT1_SIZE_BYTES);
	if (cookie.buffer == NULL) {
		bb_debug("%s cant malloc buffer\n", __func__);
		return -1;
	}

	cookie.len = BOOT1_SIZE_BYTES;
	if (!sunxi_read_uboot(&cookie, flash_type))
		needupdate = i_cb((void *)(cookie.buffer), in, item);

	if (needupdate == 0) {
		ret = sunxi_write_uboot(in, flash_type, 1);
	}

	free(cookie.buffer);
	return 0;
}

int libboot_read_boot(item_callback cb, void *out, int boot_num, void *item_name)
{
	BufferExtractCookie cookie;
	int flash_type;
	int ret = -1;

	if (!cb || !out)
		return -EINVAL;

	flash_type = getFlashType();
	cookie.buffer = (char *)malloc(BOOT1_SIZE_BYTES);
	if(cookie.buffer == NULL) {
		bb_debug("%s cant malloc buffer\n", __func__);
		return -1;
	}

	cookie.len = BOOT1_SIZE_BYTES;
	if (boot_num == BOOT0)
		ret = sunxi_read_boot0(&cookie, flash_type);
	else if(boot_num == UBOOT)
		ret = sunxi_read_uboot(&cookie, flash_type);

	if (!ret) {
		bb_debug("%s, read boot%d success\n", __func__, boot_num);
		ret = cb((char *)(cookie.buffer), out, item_name);
	}

	free(cookie.buffer);

	return ret;
}

int libboot_update_boot(item_callback cb, void *in, int boot_num, void *item)
{
	BufferExtractCookie cookie;
	int flash_type;
	int ret = -1;
	int needupdate = -1;

	if ((cb == NULL) || ((in == NULL) && (item == NULL)))
		return -EINVAL;

	flash_type = getFlashType();
	cookie.buffer = (char *)malloc(BOOT1_SIZE_BYTES);
	if (cookie.buffer == NULL) {
		bb_debug("%s cant malloc buffer\n", __func__);
		return -1;
	}
	memset(cookie.buffer, 0, BOOT1_SIZE_BYTES);

	cookie.len = BOOT1_SIZE_BYTES;
	if (boot_num == BOOT0)
		ret = sunxi_read_boot0(&cookie, flash_type);
	else if(boot_num == UBOOT)
		ret = sunxi_read_uboot(&cookie, flash_type);

	if (!ret) {
		bb_debug("%s, read boot%d success\n", __func__, boot_num);
		needupdate = cb((char *)(&cookie), in, item);
	}

	if (needupdate == 0) {
		bb_debug("%s need to do write\n", __func__);
		if (boot_num == BOOT0) {
			ret = sunxi_write_boot0(&cookie, flash_type, 1);
		}
		else if(boot_num == UBOOT) {
			ret = sunxi_write_uboot(&cookie, flash_type, 1);
		}
	}

	free(cookie.buffer);
	return 0;
}

int libboot_burn_boot(item_callback cb, void *in, int boot_num, void *item)
{
	int flash_type;
	int ret = 0;
	BufferExtractCookie cookie;

	if (!in) {
		bb_debug("%s, in is null\n", __func__);
		return -1;
	}

	flash_type = getFlashType();

	if ((item == NULL) || (cb == NULL)) {
		if (boot_num == BOOT0)
			ret = sunxi_write_boot0((BufferExtractCookie *)in, flash_type, 1);
		else if(boot_num == UBOOT)
			ret = sunxi_write_uboot((BufferExtractCookie *)in, flash_type, 1);

		if (!ret) {
				bb_debug("%s, update boot%d success\n", __func__, boot_num);
		}
		return ret;
	}

	cookie.buffer = (char *)malloc(BOOT1_SIZE_BYTES);
	if (cookie.buffer == NULL) {
		bb_debug("%s cant malloc buffer\n", __func__);
		return -1;
	}
	cookie.len = BOOT1_SIZE_BYTES;

	memset(cookie.buffer, 0, BOOT1_SIZE_BYTES);

	ret = cb((&cookie), in, item);
	if (!ret) {
		if (boot_num == BOOT0)
			ret = sunxi_write_boot0(&cookie, flash_type, 1);
		else if(boot_num == UBOOT)
			ret = sunxi_write_uboot(&cookie, flash_type, 1);

		if (!ret) {
				bb_debug("%s, update boot%d success\n", __func__, boot_num);
		}
	}
	free(cookie.buffer);
	return ret;
}


