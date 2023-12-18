/*
 * (C) Copyright 2013-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * wangwei <wangwei@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <config.h>
#include <command.h>
#include <sunxi_board.h>
#include <malloc.h>
#include <memalign.h>
#include <sunxi_flash.h>
#include <part.h>
#include <image.h>
#include <android_image.h>
#include <rtos_image.h>
#include <sys_partition.h>
#include <sprite_download.h>
#include "../sprite/sparse/sparse.h"

DECLARE_GLOBAL_DATA_PTR;

#define SUNXI_FLASH_READ_FIRST_SIZE (32 * 1024)

static int sunxi_flash_read_part(struct blk_desc *desc, disk_partition_t *info,
				 ulong buffer, ulong load_size)
{
	int ret;
	u32 rbytes, rblock, testblock;
	u32 start_block;
	u8 *addr;
	image_header_t *uz_hdr;

	addr	= (void *)buffer;
	start_block = (uint)info->start;

#ifdef CONFIG_SUNXI_RTOS
	struct rtos_img_hdr *rtos_hdr;
	rtos_hdr = (struct rtos_img_hdr *)addr;
#endif

#ifdef CONFIG_ANDROID_BOOT_IMAGE
	struct andr_img_hdr *fb_hdr;
	fb_hdr = (struct andr_img_hdr *)addr;
#endif

	testblock = SUNXI_FLASH_READ_FIRST_SIZE / 512;
	ret       = blk_dread(desc, start_block, testblock, (u_char *)buffer);
	if (ret != testblock) {
		return 1;
	}

	uz_hdr = (image_header_t *)addr;
	if (load_size)
		rbytes = load_size;
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	else if (!memcmp(fb_hdr->magic, ANDR_BOOT_MAGIC, 8)) {
		rbytes = android_image_get_end_by_avbfooter();
		//image size from avbfooter have higher priority
		if (!rbytes) {
			rbytes = android_image_get_end(fb_hdr) - (ulong)fb_hdr;

			/*secure boot img may attached with an embbed cert*/
			rbytes += sunxi_boot_image_get_embbed_cert_len(fb_hdr);
		}
	}
#endif
	else if (image_check_magic(uz_hdr)) {
		rbytes = image_get_data_size(uz_hdr) + image_get_header_size();
	}

#ifdef CONFIG_SUNXI_RTOS
	else if (!memcmp(rtos_hdr->rtos_magic, RTOS_BOOT_MAGIC, 8)) {
		rbytes = sizeof(struct rtos_img_hdr) + rtos_hdr->rtos_size;
	}
#endif
	else {
		debug("bad boot image magic, maybe not a boot.img?\n");
		rbytes = info->size * 512;
	}

	rblock = (rbytes + 511) / 512 - testblock;
	start_block += testblock;
	addr += SUNXI_FLASH_READ_FIRST_SIZE;

	ret = blk_dread(desc, start_block, rblock, (u_char *)addr);
	ret = (ret == rblock) ? 0 : 1;
	sunxi_mem_info((char *)info->name, (void *)buffer, rbytes);
	debug("sunxi flash read :offset %x, %d bytes %s\n", (u32)info->start,
	      rbytes, ret == 0 ? "OK" : "ERROR");

	return ret;
}

#include "private_boot0.h"
#include "private_toc.h"
extern int sunxi_flash_upload_boot0(char *buffer, int size, int backup_id);
void reset_boot_dram_update_flag(u32 *dram_para);
int do_sunxi_flash_boot0(cmd_tbl_t *cmdtp, int flag, int argc,
			 char *const argv[])
{

	if (argc != 3)
		return -1;
	uint8_t *boot0_buffer;
	uint32_t len;
	uint32_t *ptr_newval;
	uint32_t new_val;
	uint32_t *check_sum;
	boot0_file_head_t *boot0 = NULL;
	sbrom_toc0_config_t *toc0_config = NULL;
	boot0_buffer = malloc(1 * 1024 * 1024);
	if (!boot0_buffer) {
		pr_err("failed to malloc for boot0\n");
		return -1;
	}
	sunxi_flash_upload_boot0((char *)boot0_buffer, 1 * 1024 * 1024,
					0);

	/* Check the legitimacy of boot0/sboot */
	if (sunxi_get_secureboard() == 0) {
		boot0 = (boot0_file_head_t *)boot0_buffer;
		if (strncmp((const char *)boot0->boot_head.magic,
				BOOT0_MAGIC, MAGIC_SIZE)) {
			printf("sunxi sprite: boot0 magic is error\n");
			return -1;
		}
		len = boot0->boot_head.length;
		check_sum = &boot0->boot_head.check_sum;
	} else {
		toc0_private_head_t *toc0 =
			(toc0_private_head_t *)boot0_buffer;
		if (strncmp((const char *)toc0->name, TOC0_MAGIC,
				MAGIC_SIZE)) {
			printf("sunxi sprite: toc0 magic is error, need secure image\n");
			return -1;
		}
		len = toc0->length;
		if (toc0->items_nr == 3)
			toc0_config =
				(sbrom_toc0_config_t *)(boot0_buffer +
							0xa0);
		else
			toc0_config =
				(sbrom_toc0_config_t *)(boot0_buffer +
							0x80);
		check_sum = &toc0->check_sum;
	}

	/* get set value */
	new_val = simple_strtoul(argv[2], NULL, 16);

	/* Get the location that needs to be modified */
	if (!strcmp(argv[1], "force_dram_update_flag")) {
		if (sunxi_get_secureboard() == 0) {
			ptr_newval = boot0->prvt_head.dram_para;
		} else {
			ptr_newval = toc0_config->dram_para;
		}
		if (new_val) {
			printf("set dram flag\n");
			set_boot_dram_update_flag(ptr_newval);
		} else {
			printf("reset dram flag\n");
			reset_boot_dram_update_flag(ptr_newval);
		}
	} else if (!strcmp(argv[1], "force_dram_update_size")) {
		if (sunxi_get_secureboard() == 0) {
			ptr_newval = &boot0->dram_size;
		} else {
			ptr_newval = &toc0_config->dram_size;
		}
		*ptr_newval = new_val;
		printf("set dram %dM\n", *ptr_newval);
	} else {
		printf("do_sunxi_flash_boot0:no command\n");
		return -1;
	}

	*check_sum =
		sunxi_generate_checksum(boot0_buffer, len, 1, *check_sum);
	return sunxi_sprite_download_spl(boot0_buffer, len,
						get_boot_storage_type());

	return -1;
}

int do_sunxi_flash(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	struct blk_desc *desc;
	disk_partition_t info = { 0 };
	ulong load_addr;
	ulong load_size = 0;
	char *cmd;
	char *part_name;
	int ret;

#ifdef CONFIG_SUNXI_SPRITE
	static int partdata_format;
#endif

	if (!strcmp("boot0", argv[1])) {
		argc--;
		argv++;
		return do_sunxi_flash_boot0(cmdtp, flag, argc, argv);
	}

	/* at least four arguments please */
	if (argc < 4)
		goto usage;

#ifdef CONFIG_SUNXI_SPRITE
	else if (argc < 5)
		partdata_format = 0;
#endif

	cmd       = argv[1];
	part_name = argv[3];

	if (strncmp(cmd, "read", strlen("read")) == 0) {
		load_addr = (ulong)simple_strtoul(argv[2], NULL, 16);
		if (argc == 5)
			load_size = (ulong)simple_strtoul(argv[4], NULL, 16);
		env_set("boot_from_partion", part_name);
	}
#ifdef CONFIG_SUNXI_SPRITE
	 else if (!strncmp(cmd, "write", strlen("write"))) {
		load_addr = (ulong)simple_strtoul(argv[2], NULL, 16);
		if (!strncmp(part_name, "boot_package", strlen("boot_package")) ||
			!strncmp(part_name, "uboot", strlen("uboot")) ||
			!strncmp(part_name, "toc1", strlen("toc1"))) {
			return sunxi_sprite_download_uboot((void *)load_addr, get_boot_storage_type(), 0);
		} else if (!strncmp(part_name, "boot0", strlen("boot0")) ||
			!strncmp(part_name, "toc0", strlen("toc0"))) {
			return sunxi_sprite_download_boot0((void *)load_addr, get_boot_storage_type());
		}
#if defined(CONFIG_AW_MTD_SPINAND) || defined(CONFIG_AW_MTD_RAWNAND)
		if (strncmp(cmd, "write_mtd", strlen("write_mtd")) == 0) {
			ret = sunxi_partition_parse(part_name, &info);
			if (ret < 0)
				return -1;

			info.size = (info.size + 511) / 512;
			ret = sunxi_flash_phywrite(info.start, info.size, (void *)load_addr);
			sunxi_flash_flush();
			return ret;
		}
#endif
		/* write size: indecated on partemeter 1 */
		if (sunxi_partition_get_info_byname(part_name, (uint *)&info.start, (uint *)&info.size))
			goto usage;
		if (argc == 5) {
			/* write size: partemeter 2 */
			info.size = ALIGN((u32)simple_strtoul(argv[4], NULL, 16), 512)/512;
		} else if (argc == 6) {
			info.start += ALIGN((u32)simple_strtoul(argv[4], NULL, 16), 512)/512;
			info.size = ALIGN((u32)simple_strtoul(argv[5], NULL, 16), 512)/512;
			if (simple_strtoul(argv[4], NULL, 16) == 0) {
				partdata_format = unsparse_probe((char *)load_addr, info.size*512, (u32)info.start);
				pr_msg("partdata_format:%d\n", partdata_format);
			}
		}

		if (partdata_format != ANDROID_FORMAT_DETECT) {
			ret = sunxi_flash_write(info.start, info.size, (void *)load_addr);
		} else {
			ret = unsparse_direct_write((void *)load_addr, (u32)simple_strtoul(argv[5], NULL, 16)) ? 0 : 1;
		}

		sunxi_flash_flush();
		pr_msg("sunxi flash write :offset %lx, %d bytes %s\n", info.start, info.size*512,
				ret ? "OK" : "ERROR");

		return ret;

	}
#endif
	else {
		goto usage;
	}
#if defined(CONFIG_AW_MTD_SPINAND) || defined(CONFIG_AW_MTD_RAWNAND)
	if (strncmp(cmd, "read_mtd", strlen("read_mtd")) == 0) {
		ret = sunxi_partition_parse(part_name, &info);
		if (ret < 0)
			return -1;
		if (load_size)
			info.size = load_size;
		info.size = (info.size + 511) / 512;
		return sunxi_flash_phyread(info.start, info.size, (void *)load_addr);
	}
#endif
	desc = blk_get_devnum_by_typename("sunxi_flash", 0);
	if (desc == NULL)
		return -ENODEV;

	ret = sunxi_flash_try_partition(desc, part_name, &info);
	if (ret < 0)
		return -ENODEV;
	pr_msg("partinfo: name %s, start 0x%lx, size 0x%lx\n", info.name,
	       info.start, info.size);
	return sunxi_flash_read_part(desc, &info, load_addr, load_size);

usage:
	return cmd_usage(cmdtp);
}

U_BOOT_CMD(sunxi_flash, 6, 1, do_sunxi_flash, "sunxi_flash sub-system",
	   "sunxi_flash read mem_addr part_name [size]\n"
	   "sunxi_flash read_mtd mem_addr part_name [size]\n"
	   "sunxi_flash write <mem_addr> <part_name> [size]\n"
	   "sunxi_flash write <mem_addr> <part_name> [offset] [size]\n"
	   "sunxi_flash write_mtd <mem_addr> <part_name>\n"
	   "sunxi_flash boot0 force_dram_update_size <new_val> \n"
	   "sunxi_flash boot0 force_dram_update_flag <new_val> \n");
