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
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "libboot.h"
#include "libboot_recovery.h"
#include "sunxi_boot_api.h"
#include "private_uboot.h"
#include "private_boot0.h"
#include "private_toc.h"
#include "dtc/libdtc.h"


static char recovery_dir[128] = {'.',0};

/*
 *
 * #define BOOT_DEBUG
 *
 * */
static int get_item_form_uboot(void *buffer, void *out, void *item_name)
{
	int ret = -1;
	unsigned int i;
	char *item_head_t = NULL;
	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;
	struct sbrom_toc1_item_info  *toc1_item = NULL;

	BufferExtractCookie *out_item = NULL;

	toc1_head = (struct sbrom_toc1_head_info  *)buffer;
	item_head = (struct sbrom_toc1_item_info  *)((char *)buffer + sizeof(struct sbrom_toc1_head_info));

	out_item = out;
	if (item_name == NULL) {
		bb_debug("%s item_name is null\n", __func__);
		return -1;
	}
	bb_debug("%s %s start\n", __func__, (char *)item_name);

#if defined(BOOT_DEBUG)
	bb_debug("****%s******TOC1 Head Message*************\n", __func__);
    bb_debug("Toc_name          = %s\n",   toc1_head->name);
    bb_debug("Toc_magic         = 0x%x\n", toc1_head->magic);
    bb_debug("Toc_add_sum       = 0x%x\n", toc1_head->add_sum);

    bb_debug("Toc_serial_num    = 0x%x\n", toc1_head->serial_num);
    bb_debug("Toc_status        = 0x%x\n", toc1_head->status);

    bb_debug("Toc_items_nr      = 0x%x\n", toc1_head->items_nr);
    bb_debug("Toc_valid_len     = 0x%x\n", toc1_head->valid_len);
    bb_debug("TOC_MAIN_END      = 0x%x\n", toc1_head->end);
    bb_debug("***********************************************\n\n");
#endif

	if(toc1_head->magic != TOC_MAIN_INFO_MAGIC)
	{
		bb_debug("sunxi sprite: toc magic is error\n");
		return -1;
	}
	toc1_item = item_head;
	for(i = 0; i < toc1_head->items_nr; i++, toc1_item++) {

#if defined(BOOT_DEBUG)
		bb_debug("\n*****%s***TOC1 Item Message*****************\n", __func__);
        bb_debug("Entry_name        = %s\n",   toc1_item->name);
     	bb_debug("Entry_data_offset = 0x%x\n", toc1_item->data_offset);
        bb_debug("Entry_data_len    = 0x%x\n", toc1_item->data_len);

        bb_debug("encrypt           = 0x%x\n", toc1_item->encrypt);
        bb_debug("Entry_type        = 0x%x\n", toc1_item->type);
        bb_debug("run_addr          = 0x%x\n", toc1_item->run_addr);
        bb_debug("index             = 0x%x\n", toc1_item->index);
        bb_debug("Entry_end         = 0x%x\n", toc1_item->end);
        bb_debug("************************************************\n\n");
#endif

        if(!strcmp(toc1_item->name, item_name)) {
        	bb_debug("get = %s, len = %d\n",   toc1_item->name, toc1_item->data_len);
			item_head_t = ((char *)buffer + toc1_item->data_offset);
			memcpy(out_item->buffer, item_head_t, toc1_item->data_len);
			out_item->len = toc1_item->data_len;
			return 0;
		}
	}
	return -1;
}

static int match_item_cookie(char *item_name, void *item_head)
{
	int i = 0;
	BufferExtractCookieItemHead *in_item_head = NULL;
	BufferExtractCookieItem *in_item = NULL;

	in_item_head = (BufferExtractCookieItemHead *)item_head;

	for (i = 0; i < (int)in_item_head->items_nr; i++)
	{
		in_item = (BufferExtractCookieItem *)(&(in_item_head->cookie_item[i]));
		if (!strncmp(item_name, in_item->name, strlen(in_item->name))) {
			return i;
		}
	}
	return -1;
}

static int recovery_update_uboot(void *cookie, void *in, void *item)
{
	int ret = -1;
	int i, j;
	unsigned int offset;
	char *item_head_t = NULL;
	struct sbrom_toc1_head_info  *toc1_head = NULL;
	struct sbrom_toc1_item_info  *item_head = NULL;

	struct sbrom_toc1_item_info  *toc1_item = NULL;

	BufferExtractCookieItemHead *in_item_head = NULL;
	BufferExtractCookieItem *in_item = NULL;
	BufferExtractCookie *in_cookie = NULL;
	BufferExtractCookie *out_cookie = NULL;

	char *buffer_new = NULL;

	if ((item == NULL) || (cookie == NULL) || (in == NULL)) {
		bb_debug("%s input is null\n", __func__);
		return -1;
	}
	in_item_head = (BufferExtractCookieItemHead *)item;
	in_cookie = (BufferExtractCookie *)in;
	out_cookie = (BufferExtractCookie *)cookie;

	toc1_head = (struct sbrom_toc1_head_info  *)(in_cookie->buffer);
	item_head = (struct sbrom_toc1_item_info  *)((char *)(in_cookie->buffer) + sizeof(struct sbrom_toc1_head_info));

    if (toc1_head->magic != TOC_MAIN_INFO_MAGIC) {
		bb_debug("%s: toc magic is error\n", __func__);
		return -1;
	}

	bb_debug("%s start\n", __func__);

	buffer_new = out_cookie->buffer;
	if (buffer_new == NULL) {
		bb_debug("%s, malloc failed\n", __func__);
		return -1;
	}

	memset(buffer_new, 0, out_cookie->len);

	offset = sizeof(struct sbrom_toc1_head_info) + toc1_head->items_nr * sizeof(struct sbrom_toc1_item_info);
	memcpy((void *)buffer_new, (void *)(in_cookie->buffer), offset);

	toc1_item = (struct sbrom_toc1_item_info *)(buffer_new + sizeof(struct sbrom_toc1_head_info));

	for(i = 0; i < (int)toc1_head->items_nr; i++,toc1_item++) {


		j = match_item_cookie(toc1_item->name, in_item_head);
		if (j >= 0) {
			in_item = &(in_item_head->cookie_item[j]);
			bb_debug("do = %s, len = %ld\n", toc1_item->name, in_item->cookie.len);
			offset = randto1k(offset);
			memcpy(buffer_new + offset, in_item->cookie.buffer, in_item->cookie.len);
			toc1_item->data_offset = offset;
			offset += in_item->cookie.len;
			offset = randto1k(offset);
			toc1_item->data_len = randto1k(in_item->cookie.len);

		} else {
			offset = randto1k(offset);
			memcpy((void *)(buffer_new + offset), (void *)((char *)(in_cookie->buffer) + toc1_item->data_offset), toc1_item->data_len);
			toc1_item->data_offset = offset;
			offset += toc1_item->data_len;

		}
	}
	toc1_head = (struct sbrom_toc1_head_info *)buffer_new;
	toc1_head->valid_len = (offset + 16 * 1024 - 1) & (~(16*1024 - 1));
	toc1_head->add_sum = generate_check_sum(buffer_new, toc1_head->valid_len, toc1_head->add_sum);
	out_cookie->len = toc1_head->valid_len;
	return 0;
}

int copy_file(char *src_file, char *dst_file)
{
	int ret = 0;
	int len = 0;
	FILE *fp_src = NULL;
	FILE *fp_dst = NULL;
	char *buff = NULL;

	struct stat statbuff;

	bb_debug("%s,before stat \n", __func__);
	if (stat(src_file, &statbuff) < 0) {
		bb_debug("%s, get file %s stat failed\n", __func__, src_file);
		return -1;
	}
	bb_debug("src_file statbuff.st_size=%lld\n", statbuff.st_size);

	buff = (char *)malloc(1024*1024);
	if (buff == NULL) {
		bb_debug("%s malloc failed\n", __func__);
		return -1;
	}
	memset(buff, 0, 1024*1024);

	fp_src = fopen(src_file, "r");
	if (fp_src == NULL) {
		bb_debug("%s fopen sunxi_bd.dtb failed\n", __func__);
		ret = -1;
		goto err;
	}

	int fp_src_fd = fileno(fp_src);
	fsync(fp_src_fd);

	len = statbuff.st_size;

	fp_dst = fopen(dst_file, "w+");
	if (fp_src == NULL) {
		bb_debug("%s fopen sunxi_bd.dtb failed\n", __func__);
		ret = -1;
		goto err;
	}

	fseek(fp_src, 0, SEEK_SET);
	fseek(fp_dst, 0, SEEK_SET);
	if (fread(buff, len, 1, fp_src) != 1) {
		bb_debug("%s, read file %s failed\n", __func__, src_file);
		ret = -1;
		goto err;
	}

	if (fwrite(buff, len, 1, fp_dst) != 1) {
		bb_debug("%s write %s failed\n", __func__, dst_file);
		ret = -1;
		goto err;
	}

err:
	if (buff) {
		free(buff);
		buff = NULL;
	}
	if (fp_src) {
		fclose(fp_src);
		fp_src = NULL;
	}
	if (fp_dst) {
		fclose(fp_dst);
		fp_dst = NULL;
	}
	return ret;
}

int redtb_with_bd_cfg(void)
{
	int ret = 0;
	struct sunxi_dtc_param dtc_param;
	char inname[156] = {0};
	char outname[156] = {0};
	char fexname[156] = {0};
	char dtbname[156] = {0};

	sprintf(inname, "%s/%s", recovery_dir, "sunxi.dtb");
	sprintf(outname, "%s/%s", recovery_dir, "new_sunxi.dtb");
	sprintf(fexname, "%s/%s", recovery_dir, "board-cfg.fex");
	sprintf(dtbname, "%s/%s", recovery_dir, "sunxi_bd.dtb");

	dtc_param.inname = inname;
	dtc_param.outname = outname;
	dtc_param.fexname = fexname;

	ret = sunxi_libdtc(&dtc_param);
	if (ret) {
		bb_debug("sunxi_libdtc work failed\n");
		return -1;
	}
	clearPageCache();

	copy_file(outname, dtbname);
	return 0;
}

int libboot_upate_dtb_with_bd_cfg(BufferExtractCookie *dtb, BufferExtractCookie *bd_cfg)
{
	int ret = 0;
	FILE *fp_dtb = NULL;
	FILE *fp_bd = NULL;
	char dtb_name[156] = {0};
	char fex_name[156] = {0};

	sprintf(dtb_name, "%s/%s", recovery_dir, "sunxi.dtb");
	bb_debug("dtb_name=%s\n", dtb_name);

	fp_dtb = fopen(dtb_name, "w+");
	if (fp_dtb == NULL) {
		bb_debug("%s fopen dtb failed\n", __func__);
		return -1;
	}
	fseek(fp_dtb, 0, SEEK_SET);
	if (fwrite(dtb->buffer, dtb->len, 1, fp_dtb) != 1) {
		bb_debug("%s write dtb failed\n", __func__);
		fclose(fp_dtb);
		fp_dtb = NULL;
		return -1;
	}

	fclose(fp_dtb);
	fp_dtb = NULL;


	sprintf(fex_name, "%s/%s", recovery_dir, "board-cfg.fex");
	bb_debug("fex_name=%s\n", fex_name);
	fp_bd = fopen(fex_name, "w+");
	if (fp_bd == NULL) {
		bb_debug("%s fopen board-cfg.fex failed\n", __func__);
		return -1;
	}
	fseek(fp_bd, 0, SEEK_SET);
	if (fwrite(bd_cfg->buffer, bd_cfg->len, 1, fp_bd) != 1) {
		bb_debug("%s write board-cfg.fex failed\n", __func__);
		fclose(fp_bd);
		fp_bd = NULL;
		return -1;
	}
	fclose(fp_bd);
	fp_bd = NULL;

	ret = redtb_with_bd_cfg();
	if (ret)
	{
		bb_debug("combine dtb and board-cfg file failed\n");
		return -1;
	}

	return ret;
}

int libboot_sync_dtb_of_file(BufferExtractCookie *dtb)
{
	int ret = -1;
	char dtb_name[156] = {0};

	sprintf(dtb_name, "%s/%s", recovery_dir, "sunxi_bd.dtb");
	ret = read_file_to_cookie(dtb_name, dtb);
	if (ret) {
		bb_debug("reread dtb file to cookie failed\n");
		return -1;
	}
	bb_debug("sunxi_bd->len=%ld\n", dtb->len);

	return ret;
}

int libbot_sync_bd_cfg_of_file(BufferExtractCookie *bd_cfg)
{
	int ret = -1;
	char bdcfg_name[156] = {0};

	sprintf(bdcfg_name, "%s/%s", recovery_dir, "board_config.bin");

	ret = read_file_to_cookie(bdcfg_name, bd_cfg);
	if (ret) {
		bb_debug("reread board-cfg file to cookie failed\n");
		return -1;
	}
	return ret;
}

int libboot_update_boot_without_bdcfg(BufferExtractCookie *in)
{
	int ret = -1;

	BufferExtractCookieItemHead bd_item_head;
	char *bd_cfg_buff = NULL;
	char *bd_cfg_bin_buff = NULL;
	char *dtb_buff = NULL;

	bd_cfg_buff = (char *)malloc(1 * 1024 *1024);
	if (bd_cfg_buff == NULL) {
		bb_debug("%s malloc bd_cfg_buff falied\n", __func__);
		return -1;
	}
	memset(bd_cfg_buff, 0, 1*1024*1024);

	strncpy(bd_item_head.name, BUFFER_ITEM_COOKIE_MAGIC, sizeof(BUFFER_ITEM_COOKIE_MAGIC));
	bd_item_head.items_nr = 0;
	bd_item_head.valid_len = sizeof(BufferExtractCookieItemHead);

	strncpy(bd_item_head.cookie_item[0].name, ITEM_BDCFG_FEX_NAME, sizeof(ITEM_BDCFG_FEX_NAME));

	bd_item_head.cookie_item[0].cookie.buffer = bd_cfg_buff;
	bd_item_head.cookie_item[0].cookie.len = 1024 * 1024;

	//read board-cfg.fex form flash
	ret = libboot_read_boot(get_item_form_uboot, &(bd_item_head.cookie_item[0].cookie), 1, ITEM_BDCFG_FEX_NAME);
	if (ret) {
		bb_debug("%s read %s falied\n", __func__, ITEM_BDCFG_FEX_NAME);
		goto err;
	}
	bd_item_head.items_nr = 1;

	bd_cfg_bin_buff = (char *)malloc(1 * 1024 *1024);
	if (bd_cfg_bin_buff == NULL) {
		bb_debug("%s malloc bd_cfg_bim_buff falied\n", __func__);
		return -1;
	}
	memset(bd_cfg_bin_buff, 0, 1*1024*1024);

	strncpy(bd_item_head.cookie_item[1].name, ITEM_BDCFG_NAME, sizeof(ITEM_BDCFG_NAME));
	bd_item_head.cookie_item[1].cookie.buffer = bd_cfg_bin_buff;
	bd_item_head.cookie_item[1].cookie.len = 1024 * 1024;

	//read board-cfg.bin form flash
	ret = libboot_read_boot(get_item_form_uboot, &(bd_item_head.cookie_item[1].cookie), 1, ITEM_BDCFG_NAME);
	if (ret) {
		bb_debug("%s read %s falied\n", __func__, ITEM_BDCFG_NAME);
		goto err;
	}
	bd_item_head.items_nr = 2;

	dtb_buff = (char *)malloc(2 * 1024 *1024);
	if (dtb_buff == NULL) {
		bb_debug("%s malloc dtb buffer failed\n", __func__);
		goto err;
	}
	memset(dtb_buff, 0, 2*1024*1024);

	strncpy(bd_item_head.cookie_item[2].name, ITEM_DTB_NAME, sizeof(ITEM_DTB_NAME));
	bd_item_head.cookie_item[2].cookie.buffer = dtb_buff;
	bd_item_head.cookie_item[2].cookie.len = 2 * 1024 * 1024;
	//read dtb form input
	ret = get_item_form_uboot(in->buffer, &(bd_item_head.cookie_item[2].cookie), ITEM_DTB_NAME);
	if (ret) {
		bb_debug("%s read %s falied\n", __func__, ITEM_DTB_NAME);
		goto err;
	}

	//update dtb with board-cfg.fex
	ret = libboot_upate_dtb_with_bd_cfg(&(bd_item_head.cookie_item[2].cookie), &(bd_item_head.cookie_item[0].cookie));
	if (ret) {
		bb_debug("%s update %s by bd_cfg falied\n", __func__, ITEM_DTB_NAME);
		goto err;
	}

	memset(dtb_buff, 0, 2 * 1024 * 1024);
	bd_item_head.cookie_item[2].cookie.len = 2 * 1024 * 1024;

	ret = libboot_sync_dtb_of_file(&(bd_item_head.cookie_item[2].cookie));
	if (ret) {
		bb_debug("%s read %s from file falied\n", __func__, ITEM_DTB_NAME);
		goto err;
	}

	bd_item_head.items_nr = 3;
	ret = libboot_burn_boot(recovery_update_uboot, in, 1, &bd_item_head);
	if (ret) {
		bb_debug("%s update boot1 without board_cfg failed\n", __func__);
		goto err;
	}
	bb_debug("%s success\n", __func__);

err:
	if (bd_cfg_bin_buff) {
		free(bd_cfg_bin_buff);
		bd_cfg_bin_buff = NULL;
	}

	if (bd_cfg_buff) {
		free(bd_cfg_buff);
		bd_cfg_buff = NULL;
	}

	if (dtb_buff) {
		free(dtb_buff);
		dtb_buff = NULL;
	}
	return ret;
}

int recovery_update_board_cfg(void *cookie, void *in, void *item_head)
{
	int ret = -1;
	char *dtb_buff = NULL;
	char *bd_cfg_bin_buff = NULL;

	BufferExtractCookie new_cookie;
	BufferExtractCookie *uboot_cookie = NULL;
	BufferExtractCookie *bd_cookie = NULL;
	BufferExtractCookieItemHead *bd_item_head = NULL;

	if ((cookie == NULL) || (in == NULL) || (item_head == NULL))
		return -1;

	bd_item_head = (BufferExtractCookieItemHead *)item_head;
	uboot_cookie = (BufferExtractCookie *)cookie;
	bd_cookie = (BufferExtractCookie *)in;

	dtb_buff = (char *)malloc(2 * 1024 *1024);
	if (dtb_buff == NULL) {
		bb_debug("%s malloc dtb buff failed\n", __func__);
		return -1;
	}
	memset(dtb_buff, 0, 2 * 1024 * 1024);

	strncpy(bd_item_head->cookie_item[0].name, ITEM_DTB_NAME, sizeof(ITEM_DTB_NAME));
	bd_item_head->cookie_item[0].cookie.buffer = dtb_buff;
	bd_item_head->cookie_item[0].cookie.len = 2 * 1024 * 1024;

	//get old dtb
	ret = get_item_form_uboot(uboot_cookie->buffer, &(bd_item_head->cookie_item[0].cookie), ITEM_DTB_NAME);
	if (ret) {
		bb_debug("%s get dtb failed\n", __func__);
		goto err;
	}

	//update dtb with board-cfg.fex
	ret = libboot_upate_dtb_with_bd_cfg(&(bd_item_head->cookie_item[0].cookie), in);
	if (ret) {
		bb_debug("%s update %s by bd_cfg falied\n", __func__, ITEM_DTB_NAME);
		goto err;
	}

	memset(dtb_buff, 0, 2 * 1024 * 1024);

	//get new dtb
	ret = libboot_sync_dtb_of_file(&(bd_item_head->cookie_item[0].cookie));
	if (ret) {
		bb_debug("%s read %s from file falied\n", __func__, ITEM_DTB_NAME);
		goto err;
	}
	bd_item_head->items_nr = 1;

	strncpy(bd_item_head->cookie_item[1].name, ITEM_BDCFG_FEX_NAME, sizeof(ITEM_BDCFG_FEX_NAME));
	bd_item_head->cookie_item[1].cookie.buffer = bd_cookie->buffer;
	bd_item_head->cookie_item[1].cookie.len = bd_cookie->len;
	bd_item_head->items_nr = 2;

	bd_cfg_bin_buff = (char *)malloc(1 * 1024 *1024);
	if (bd_cfg_bin_buff == NULL) {
		bb_debug("%s malloc bd_cfg_bin_buff failed\n", __func__);
		goto err;
	}
	memset(bd_cfg_bin_buff, 0, 1 * 1024 * 1024);
	strncpy(bd_item_head->cookie_item[2].name, ITEM_BDCFG_NAME, sizeof(ITEM_BDCFG_NAME));
	bd_item_head->cookie_item[2].cookie.buffer = bd_cfg_bin_buff;
	bd_item_head->cookie_item[2].cookie.len = 1024 * 1024;
	ret = libbot_sync_bd_cfg_of_file(&(bd_item_head->cookie_item[2].cookie));
	if (ret) {
		bb_debug("%s read %s form file falied\n", __func__, ITEM_BDCFG_NAME);
		goto err;
	}
	bd_item_head->items_nr = 3;

	new_cookie.buffer = (char *)malloc(BOOT1_SIZE_BYTES);
	if (new_cookie.buffer == NULL) {
		bb_debug("%s malloc new_cookie.buffer failed\n", __func__);
		return -1;
	}
	new_cookie.len = BOOT1_SIZE_BYTES;
	memset(new_cookie.buffer, 0, BOOT1_SIZE_BYTES);

	ret = recovery_update_uboot(&new_cookie, uboot_cookie, bd_item_head);
	if (ret) {
		bb_debug("%s restore uboot failed\n", __func__);
		goto err;
	}
	memset(uboot_cookie->buffer, 0, uboot_cookie->len);
	memcpy(uboot_cookie->buffer, new_cookie.buffer, new_cookie.len);
	bb_debug("%s success\n", __func__);
err:
	if (dtb_buff) {
		free(dtb_buff);
		dtb_buff = NULL;
	}

	if (bd_cfg_bin_buff) {
		free(bd_cfg_bin_buff);
		bd_cfg_bin_buff = NULL;
	}

	if (new_cookie.buffer) {
		free(new_cookie.buffer);
		new_cookie.buffer = NULL;
	}
	return ret;
}

int libboot_update_board_cfg(BufferExtractCookie *in)
{
	int ret = -1;
	BufferExtractCookieItemHead bd_item_head;
	ret = libboot_update_boot(recovery_update_board_cfg, in, 1, &bd_item_head);
	if (ret) {
		bb_debug("%s update failed\n", __func__);
	}
	return ret;
}

int libboot_recovery_set_file_dir(BufferExtractCookie *dir_cookie)
{
	memset(recovery_dir, 0, sizeof(recovery_dir));
	if ((dir_cookie->len > (long)(sizeof(recovery_dir) - 1) || (dir_cookie->len) < 1)) {
		bb_debug("the dir string is to long, %ld\n", dir_cookie->len);
		return -1;
	}
	strncpy(recovery_dir, dir_cookie->buffer, dir_cookie->len);
	bb_debug("recovery_dir is %s\n", recovery_dir);
	return 0;
}

int libboot_recovery_read_uboot_item(BufferExtractCookie *out, void *item_name)
{
	return libboot_read_boot(get_item_form_uboot, out, 1, item_name);
}

static int libboot_recovery_burn_boot0(BufferExtractCookie *in)
{
	int flash_type = 0;
	int ret = -1;
	struct user_mmc_para mmc_para;

	flash_type = getFlashType();
	if((flash_type == FLASH_TYPE_SD1) || (flash_type == FLASH_TYPE_SD2)) {
		ret = libboot_read_mmc_param(&mmc_para);
		if (ret) {
			bb_debug("%s read mmc para failed\n", __func__);
			return -1;;
		}

		ret = libboot_burn_mmc_para(in, &mmc_para);
		if (ret) {
			bb_debug("%s burn boot0 failed\n", __func__);
			return -1;
		}
	} else {
		ret = libboot_burn_boot(NULL, in, 0, NULL);
		if (ret) {
			bb_debug("%s burn boot0 failed\n", __func__);
			return -1;
		}
	}

	return ret;
}

int libboot_recovery_burn_boot(BufferExtractCookie *in, void *item_name)
{
	int ret = -1;

	if (!strncmp(item_name, ITEM_BOOT0_NAME, sizeof(ITEM_BOOT0_NAME)) ||
			!strncmp(item_name, ITEM_TOC0_NAME, sizeof(ITEM_TOC0_NAME))) {
		//burn boot0 or toc0, we must read mmc para from old flash
		ret = libboot_recovery_burn_boot0(in);
		if (ret) {
			bb_debug("%s burn boot0 failed\n", __func__);
		}
		return ret;
	} else if (!strncmp(item_name, ITEM_TOC1_NAME, sizeof(ITEM_TOC1_NAME)) ||
		!strncmp(item_name, ITEM_BOOT_PACKAGE_NAME, sizeof(ITEM_BOOT_PACKAGE_NAME))) {
		//burn toc1 or boot_package without board-cfg
		bb_debug("%s burn toc1 without board-cfg\n", __func__);
		ret = libboot_update_boot_without_bdcfg(in);
		return ret;
	}
	else if (!strncmp(item_name, ITEM_BDCFG_NAME, sizeof(ITEM_BDCFG_NAME))) {
		bb_debug("%s burn board-cfg\n", __func__);
		//burn board-cfg.fex board-cfg.bin
		ret = libboot_update_board_cfg(in);
		if (ret) {
			bb_debug("update board-cfg falied\n");
		}
		return ret;
	} else {
		bb_debug("%s unsupport %s", __func__, item_name);
	}
	return ret;
}

int updateToc0(const char* path) {
	int ret = -1;
	BufferExtractCookie cookie;
	ret = get_cookie_of_file(path, &cookie);
	if (ret) {
		bb_debug("can't open %s\n", path);
		return -1;
	}
	return libboot_recovery_burn_boot(&cookie, ITEM_TOC0_NAME);
}
int updateToc1(const char* path) {
	int ret = -1;
	BufferExtractCookie cookie;
	ret = get_cookie_of_file(path, &cookie);
	if (ret) {
		bb_debug("can't open %s\n", path);
		return -1;
	}
	return libboot_recovery_burn_boot(&cookie, ITEM_TOC1_NAME);
}


int updateBoot0(const char* path) {
	int ret = -1;
	BufferExtractCookie cookie;
	ret = get_cookie_of_file(path, &cookie);
	if (ret) {
		bb_debug("can't open %s\n", path);
		return -1;
	}
	return libboot_recovery_burn_boot(&cookie, ITEM_BOOT0_NAME);
}

int updateUboot(const char* path) {
	int ret = -1;
	BufferExtractCookie cookie;
	ret = get_cookie_of_file(path, &cookie);
	if (ret) {
		bb_debug("can't open %s\n", path);
		return -1;
	}
	return libboot_recovery_burn_boot(&cookie, ITEM_BOOT_PACKAGE_NAME);
}



int updateBoardConfig(const char* path) {
	int ret = -1;
	BufferExtractCookie cookie;
	ret = get_cookie_of_file(path, &cookie);
	if (ret) {
		bb_debug("can't open %s\n", path);
		return -1;
	}
	return libboot_recovery_burn_boot(&cookie, ITEM_BDCFG_NAME);
}

int setIntermediaDir(const char* path) {
	int res;
	BufferExtractCookie dir_cookie;
	mkdir(path,0755);
	dir_cookie.buffer = strdup(path);
	dir_cookie.len = strlen(path);
	res = libboot_recovery_set_file_dir(&dir_cookie);
	if (dir_cookie.buffer) {
		free(dir_cookie.buffer);
	}
	return res;
}

