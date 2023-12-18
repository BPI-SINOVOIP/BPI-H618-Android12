/*
 * (C) Copyright 2022-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * lujianliang <lujianliang@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <android_image.h>
#include <rtos_image.h>
#include <sunxi_board.h>
#include <jffs2/jffs2.h>
#include <linux/mtd/aw-spinand.h>

#if defined(CONFIG_AW_MTD_SPINAND) || defined(CONFIG_AW_MTD_RAWNAND)
static int nand_load_image(cmd_tbl_t *cmdtp, struct part_info *part,
			ulong addr, char *cmd)
{
	int ret;
	size_t len;
	image_header_t *uz_hdr;

#ifdef CONFIG_SUNXI_RTOS
	struct rtos_img_hdr *rtos_hdr;
	rtos_hdr = (struct rtos_img_hdr *)addr;
#endif

#ifdef CONFIG_ANDROID_BOOT_IMAGE
	struct andr_img_hdr *fb_hdr;
	fb_hdr = (struct andr_img_hdr *)addr;
#endif

	printf("\nLoading from %s\n", part->name);

	ret = sunxi_flash_phyread(part->offset, 4, (u_char *)addr);
	if (ret) {
		puts("** Read error\n");
		return 1;
	}

	uz_hdr = (image_header_t *)addr;
	if (image_check_magic(uz_hdr))
		len = image_get_data_size(uz_hdr) + image_get_header_size();
#ifdef CONFIG_ANDROID_BOOT_IMAGE
	else if (!memcmp(fb_hdr->magic, ANDR_BOOT_MAGIC, 8)) {
		len = android_image_get_end(fb_hdr) - (ulong)fb_hdr;

		/*secure boot img may attached with an embbed cert*/
		len += sunxi_boot_image_get_embbed_cert_len(fb_hdr);
	}
#endif
#ifdef CONFIG_SUNXI_RTOS
	else if (!memcmp(rtos_hdr->rtos_magic, RTOS_BOOT_MAGIC, 8)) {
		len = sizeof(struct rtos_img_hdr) + rtos_hdr->rtos_size;
	}
#endif
	else {
		debug("bad boot image magic, maybe not a boot.img?\n");
		len = part->size;
	}

	ret = sunxi_flash_phyread(part->offset, len >> 9, (u_char *)addr);
	if (ret) {
		puts("** Read error\n");
		return 1;
	}

	/* Loading ok, update default load address */

	load_addr = addr;

	return bootm_maybe_autostart(cmdtp, cmd);
}

static int do_nandboot(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	char *boot_device = NULL;
	int idx;
	ulong addr, offset = 0;
	char *name;
	struct mtd_device *dev;
	struct part_info *part;
	u8 pnum;
#if defined(CONFIG_CMD_MTDPARTS)
	if (argc >= 2) {
		name = (argc == 2) ? argv[1] : argv[2];
		if (!(str2long(name, &addr)) && (mtdparts_init() == 0) &&
		    (find_dev_and_part(name, &dev, &pnum, &part) == 0)) {
			if (dev->id->type != MTD_DEV_TYPE_NAND) {
				puts("Not a NAND device\n");
				return CMD_RET_FAILURE;
			}
			if (argc > 3)
				goto usage;
			if (argc == 3)
				addr = simple_strtoul(argv[1], NULL, 16);
			else
				addr = CONFIG_SYS_LOAD_ADDR;

			return nand_load_image(cmdtp, part, addr, argv[0]);
		}
	}
#endif

	switch (argc) {
	case 1:
		addr = CONFIG_SYS_LOAD_ADDR;
		boot_device = env_get("bootdevice");
		break;
	case 2:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = env_get("bootdevice");
		break;
	case 3:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = argv[2];
		break;
	case 4:
		addr = simple_strtoul(argv[1], NULL, 16);
		boot_device = argv[2];
		offset = simple_strtoul(argv[3], NULL, 16);
		break;
	default:
#if defined(CONFIG_CMD_MTDPARTS)
usage:
#endif
		return CMD_RET_USAGE;
	}

	if (!boot_device) {
		puts("\n** No boot device **\n");
		return CMD_RET_USAGE;
	}

	idx = simple_strtoul(boot_device, NULL, 16);

	if (mtdparts_init()) {
		puts("mtdpart init error\n");
		return CMD_RET_FAILURE;
	}
	name = sunxi_get_mtdparts_name(idx);

	if (find_dev_and_part(name, &dev, &pnum, &part))
		return CMD_RET_FAILURE;

	if (dev->id->type != MTD_DEV_TYPE_NAND) {
		puts("Not a NAND device\n");
		return CMD_RET_FAILURE;
	}

	/*
	 * Get partition offset data.
	 * The read cannot exceed the partition size.
	 */
	part->offset += offset;
	part->size -= offset;

	return nand_load_image(cmdtp, part, addr, argv[0]);
}
#else
static int do_nandboot(cmd_tbl_t *cmdtp, int flag, int argc,
		       char * const argv[])
{
	return 0;
}
#endif
U_BOOT_CMD(nboot, 4, 1, do_nandboot,
	"boot from AW NAND device",
	"[memaddr] <partition> | [[[loadAddr] dev] offset]"
);
