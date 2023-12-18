// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2000-2009
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/*
 * Boot support
 */
#include <common.h>
#include <bootm.h>
#include <command.h>
#include <environment.h>
#include <errno.h>
#include <image.h>
#include <malloc.h>
#include <nand.h>
#include <asm/byteorder.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <u-boot/zlib.h>
#include <sunxi_image_verifier.h>
#include <sunxi_board.h>
#include <android_image.h>
#include <fdt_support.h>
#include <sunxi_eink.h>

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_CMD_IMI)
static int image_info(unsigned long addr);
#endif

#if defined(CONFIG_CMD_IMLS)
#include <flash.h>
#include <mtd/cfi_flash.h>
extern flash_info_t flash_info[]; /* info for FLASH chips */
#endif

#if defined(CONFIG_CMD_IMLS) || defined(CONFIG_CMD_IMLS_NAND)
static int do_imls(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
#endif

/* we overload the cmd field with our state machine info instead of a
 * function pointer */
static cmd_tbl_t cmd_bootm_sub[] = {
	U_BOOT_CMD_MKENT(start, 0, 1, (void *)BOOTM_STATE_START, "", ""),
	U_BOOT_CMD_MKENT(loados, 0, 1, (void *)BOOTM_STATE_LOADOS, "", ""),
#ifdef CONFIG_SYS_BOOT_RAMDISK_HIGH
	U_BOOT_CMD_MKENT(ramdisk, 0, 1, (void *)BOOTM_STATE_RAMDISK, "", ""),
#endif
#ifdef CONFIG_OF_LIBFDT
	U_BOOT_CMD_MKENT(fdt, 0, 1, (void *)BOOTM_STATE_FDT, "", ""),
#endif
	U_BOOT_CMD_MKENT(cmdline, 0, 1, (void *)BOOTM_STATE_OS_CMDLINE, "", ""),
	U_BOOT_CMD_MKENT(bdt, 0, 1, (void *)BOOTM_STATE_OS_BD_T, "", ""),
	U_BOOT_CMD_MKENT(prep, 0, 1, (void *)BOOTM_STATE_OS_PREP, "", ""),
	U_BOOT_CMD_MKENT(fake, 0, 1, (void *)BOOTM_STATE_OS_FAKE_GO, "", ""),
	U_BOOT_CMD_MKENT(go, 0, 1, (void *)BOOTM_STATE_OS_GO, "", ""),
};

static int do_bootm_subcommand(cmd_tbl_t *cmdtp, int flag, int argc,
			char * const argv[])
{
	int ret = 0;
	long state;
	cmd_tbl_t *c;

	c = find_cmd_tbl(argv[0], &cmd_bootm_sub[0], ARRAY_SIZE(cmd_bootm_sub));
	argc--; argv++;

	if (c) {
		state = (long)c->cmd;
		if (state == BOOTM_STATE_START)
			state |= BOOTM_STATE_FINDOS | BOOTM_STATE_FINDOTHER;
	} else {
		/* Unrecognized command */
		return CMD_RET_USAGE;
	}

	if (((state & BOOTM_STATE_START) != BOOTM_STATE_START) &&
	    images.state >= state) {
		printf("Trying to execute a command out of order\n");
		return CMD_RET_USAGE;
	}

	ret = do_bootm_states(cmdtp, flag, argc, argv, state, &images, 0);

	return ret;
}

#if CONFIG_SUNXI_INITRD_ROUTINE
__maybe_unused static void sunxi_update_initrd(ulong os_load_addr)
{
	const struct andr_img_hdr *hdr =
		(const struct andr_img_hdr *)os_load_addr;
	ulong initrd_start;
	ulong initrd_size;
	unsigned long ramdisk_addr = env_get_hex("load_ramdisk_addr", hdr->ramdisk_addr);
	android_image_get_ramdisk(hdr, &initrd_start, &initrd_size);
	if ((!strncmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) && (hdr->unused >= 0x3)) {
		ulong vendor_initrd_start, vendor_initrd_size;
		struct vendor_boot_img_hdr *vendor_hdr = get_vendor_hdr_addr();
		ramdisk_addr = env_get_hex("load_ramdisk_addr", vendor_hdr->ramdisk_addr);
		android_image_get_vendor_ramdisk(hdr, &vendor_initrd_start, &vendor_initrd_size);
		memmove_wd((void *)(ramdisk_addr), (void *)vendor_initrd_start, vendor_initrd_size, CHUNKSZ);
		memmove_wd((void *)(ramdisk_addr + vendor_initrd_size), (void *)initrd_start, initrd_size, CHUNKSZ);

		initrd_size += vendor_initrd_size;

		/*bootconfig exist*/
		if (android_image_is_vendor_bootconfig_used(vendor_hdr)) {
			ulong bootconfig_start = 0;
			ulong bootconfig_size  = 0;
			android_image_get_vendor_bootconfig(
				hdr, &bootconfig_start, &bootconfig_size);
			android_image_cmdline_to_vendorbootconfig(&bootconfig_start, &bootconfig_size);
			memmove_wd((void *)(ramdisk_addr + initrd_size),
				   (void *)bootconfig_start, bootconfig_size,
				   CHUNKSZ);
			initrd_size += bootconfig_size;
			env_set_hex("bootconfig_addr", bootconfig_start);
			env_set_hex("bootconfig_size", bootconfig_size);
		}

		if (vendor_hdr->dtb_addr) {
			env_set_hex("load_dtb_addr", env_get_hex("load_dtb_addr", vendor_hdr->dtb_addr));
		}
	} else {
		if ((!strncmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) && (hdr->dtb_addr)) {
			env_set_hex("load_dtb_addr", env_get_hex("load_dtb_addr", hdr->dtb_addr));
		}
		memmove_wd((void *)ramdisk_addr, (void *)initrd_start, initrd_size, CHUNKSZ);
	}
	sunxi_mem_info("ramdisk", (void *)ramdisk_addr, initrd_size);
	env_set_hex("ramdisk_sum",
		    sunxi_generate_checksum((void *)ramdisk_addr, initrd_size,
					    8, STAMP_VALUE));
	debug("   Loading Ramdisk from %08lx to %08lx, size %08lx ... ",
		initrd_start, ramdisk_addr, initrd_size);
#ifdef CONFIG_MP
	/*
			 * Ensure the image is flushed to memory to handle
			 * AMP boot scenarios in which we might not be
			 * HW cache coherent
			 */
	flush_cache((unsigned long)*initrd_start,
		    ALIGN(rd_len, ARCH_DMA_MINALIGN));
#endif
	debug("OK\n");
	env_set_hex("ramdisk_start", ramdisk_addr);
	env_set_hex("ramdisk_size", initrd_size);
	if (initrd_size) {
		fdt_initrd(working_fdt, (ulong)ramdisk_addr,
			(ulong)(ramdisk_addr + initrd_size));
	}


}
#endif

/*
 * depends on boot header, not correct with sunxi-compressed kernel(no header)
 * maybe not always accurate, but good enough in our use case:
 *     run 32 bit os in 64 bit platform for memory usage reducing
 */
__maybe_unused static void check_os_run_arch(ulong os_load_addr)
{
	andr_img_hdr *hdr		       = (andr_img_hdr *)os_load_addr;
	struct vendor_boot_img_hdr *vendor_hdr = get_vendor_hdr_addr();
	const char *active_name		       = NULL;

	/*default value*/
	sunxi_set_force_32bit_os(0);

	/*32 bit platform(no monitor) always 32bit os*/
	if (!sunxi_probe_secure_monitor())
		return;

	/*try a valid head, either vendor boot or android boot is acceptable*/
	if (vendor_hdr != NULL) {
		active_name = (char *)vendor_hdr->name;
#if 0 //debug print
		pr_msg("vendor boot header\n");
		printf("kernel_addr:%x kernel_size:%x\n"
		       "dtb_addr:%llx dtb_size:%x\n"
		       "ramdisk_addr:%x ramdisk_size:%x\n",
		       vendor_hdr->kernel_addr, hdr->kernel_size,
		       vendor_hdr->dtb_addr, vendor_hdr->dtb_size,
		       vendor_hdr->ramdisk_addr,
		       vendor_hdr->vendor_ramdisk_size);
		pr_msg("boot header\n");
		printf("kernel_addr:%x kernel_size:%x\n"
		       "dtb_addr:%llx dtb_size:%x\n"
		       "ramdisk_addr:%x ramdisk_size:%x\n",
		       hdr->kernel_addr, hdr->kernel_size, hdr->dtb_addr,
		       hdr->dtb_size, hdr->ramdisk_addr, hdr->ramdisk_size);
#endif
	} else if (memcmp(ANDR_BOOT_MAGIC, hdr->magic, ANDR_BOOT_MAGIC_SIZE) ==
		   0) {
		active_name = hdr->name;
	} else {
		return;
	}

	/*os 64 bit*/
	if (strstr(active_name, "arm64") != NULL)
		return;

	if (strstr(active_name, "arm") != NULL) {
		pr_msg("force run 32bit os on this device\n");
		sunxi_set_force_32bit_os(1);
	}
}

/*******************************************************************/
/* bootm - boot application image from image in memory */
/*******************************************************************/
void update_bootargs(void);
int sunxi_get_lcd_op_finished(void);
int do_bootm(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong os_load_addr = simple_strtoul(argv[1], NULL, 16);
	int fake_go = 0;
	if (env_get("fake_go"))
		fake_go = BOOTM_STATE_OS_FAKE_GO;

#ifdef CONFIG_NEEDS_MANUAL_RELOC
	static int relocated = 0;

	if (!relocated) {
		int i;

		/* relocate names of sub-command table */
		for (i = 0; i < ARRAY_SIZE(cmd_bootm_sub); i++)
			cmd_bootm_sub[i].name += gd->reloc_off;

		relocated = 1;
	}
#endif

#ifdef CONFIG_SUNXI_ANDROID_BOOT
	if (sunxi_android_boot(env_get("boot_from_partion"),
			       os_load_addr)) {
		return -1;
	}
#else

#ifdef CONFIG_SUNXI_SECURE_BOOT
	/* verify image before booting in secure boot*/
	if (gd->securemode) {

#if defined(CONFIG_SUNXI_DM_VERITY)
		if (sunxi_verity_hash_tree("rootfs", "rootfs") != 0) {
			pr_err("sunxi verity rootfs error\n");
			return -1;
		}
#elif defined(CONFIG_SUNXI_PART_VERIFY)
		pr_msg("begin to verify rootfs\n");
		int full = 0;
		struct sunxi_image_verify_pattern_st verify_pattern = {
			0x1000, 0x100000, -1
		};
		char *rootfs_name = env_get("root_partition");
		char *s = env_get("rootfs_per_MB");
		if (strcmp(s, "full") == 0) {
			full = 1;
		} else {
			int rootfs_per_MB =
				s ? (int)simple_strtol(s, NULL, 10) : 0;
			if ((rootfs_per_MB >= 4096 && rootfs_per_MB <= 1048576)
				 && (rootfs_per_MB % 4096 == 0))
				verify_pattern.size = rootfs_per_MB;
			else if (rootfs_per_MB != 0) {
				return -1;
			}

			full = 0;
		}
		if (sunxi_verify_partion(&verify_pattern, rootfs_name, "rootfs", full) != 0) {
			return -1;
		}
#endif /*CONFIG_SUNXI_PART_VERIFY*/

		if (sunxi_verify_os(os_load_addr,
				    env_get("boot_from_partion")) != 0) {
			return -1;
		}
	}
#endif /*CONFIG_SUNXI_SECURE_BOOT*/

#endif /*CONFIG_SUNXI_ANDROID_BOOT*/

#if defined(CONFIG_EINK200_SUNXI)
	struct eink_fb_info_t *p_inst = eink_get_fb_inst();
	if (p_inst) {
		p_inst->wait_pipe_finish(p_inst);
	}
#elif defined(CONFIG_DISP2_SUNXI)
	while (!sunxi_get_lcd_op_finished()) {
		mdelay(10);
	}
#endif

#if CONFIG_SUNXI_INITRD_ROUTINE
	sunxi_update_initrd(os_load_addr);
#endif

	check_os_run_arch(os_load_addr);

	/* determine if we have a sub command */
	argc--; argv++;
	if (argc > 0) {
		char *endp;

		simple_strtoul(argv[0], &endp, 16);
		/* endp pointing to NULL means that argv[0] was just a
		 * valid number, pass it along to the normal bootm processing
		 *
		 * If endp is ':' or '#' assume a FIT identifier so pass
		 * along for normal processing.
		 *
		 * Right now we assume the first arg should never be '-'
		 */
		if ((*endp != 0) && (*endp != ':') && (*endp != '#'))
			return do_bootm_subcommand(cmdtp, flag, argc, argv);
	}
	update_bootargs();
	update_vendorbootconfig_and_bootgargs();

	return do_bootm_states(cmdtp, flag, argc, argv, BOOTM_STATE_START |
		BOOTM_STATE_FINDOS | BOOTM_STATE_FINDOTHER |
		BOOTM_STATE_LOADOS |
#ifndef CONFIG_SUNXI_INITRD_ROUTINE
#ifdef CONFIG_SYS_BOOT_RAMDISK_HIGH
#ifndef CONFIG_SUNXI_UBIFS
		BOOTM_STATE_RAMDISK |
#endif
#endif
#endif
#if defined(CONFIG_PPC) || defined(CONFIG_MIPS)
		BOOTM_STATE_OS_CMDLINE |
#endif
		BOOTM_STATE_OS_PREP | fake_go |
		BOOTM_STATE_OS_GO, &images, 1);
}

int bootm_maybe_autostart(cmd_tbl_t *cmdtp, const char *cmd)
{
	const char *ep = env_get("autostart");

	if (ep && !strcmp(ep, "yes")) {
		char *local_args[2];
		local_args[0] = (char *)cmd;
		local_args[1] = NULL;
		printf("Automatic boot of image at addr 0x%08lX ...\n", load_addr);
		return do_bootm(cmdtp, 0, 1, local_args);
	}

	return 0;
}

#ifdef CONFIG_SYS_LONGHELP
static char bootm_help_text[] =
	"[addr [arg ...]]\n    - boot application image stored in memory\n"
	"\tpassing arguments 'arg ...'; when booting a Linux kernel,\n"
	"\t'arg' can be the address of an initrd image\n"
#if defined(CONFIG_OF_LIBFDT)
	"\tWhen booting a Linux kernel which requires a flat device-tree\n"
	"\ta third argument is required which is the address of the\n"
	"\tdevice-tree blob. To boot that kernel without an initrd image,\n"
	"\tuse a '-' for the second argument. If you do not pass a third\n"
	"\ta bd_info struct will be passed instead\n"
#endif
#if defined(CONFIG_FIT)
	"\t\nFor the new multi component uImage format (FIT) addresses\n"
	"\tmust be extended to include component or configuration unit name:\n"
	"\taddr:<subimg_uname> - direct component image specification\n"
	"\taddr#<conf_uname>   - configuration specification\n"
	"\tUse iminfo command to get the list of existing component\n"
	"\timages and configurations.\n"
#endif
	"\nSub-commands to do part of the bootm sequence.  The sub-commands "
	"must be\n"
	"issued in the order below (it's ok to not issue all sub-commands):\n"
	"\tstart [addr [arg ...]]\n"
	"\tloados  - load OS image\n"
#if defined(CONFIG_SYS_BOOT_RAMDISK_HIGH)
	"\tramdisk - relocate initrd, set env initrd_start/initrd_end\n"
#endif
#if defined(CONFIG_OF_LIBFDT)
	"\tfdt     - relocate flat device tree\n"
#endif
	"\tcmdline - OS specific command line processing/setup\n"
	"\tbdt     - OS specific bd_t processing\n"
	"\tprep    - OS specific prep before relocation or go\n"
#if defined(CONFIG_TRACE)
	"\tfake    - OS specific fake start without go\n"
#endif
	"\tgo      - start OS";
#endif

U_BOOT_CMD(
	bootm,	CONFIG_SYS_MAXARGS,	1,	do_bootm,
	"boot application image from memory", bootm_help_text
);

/*******************************************************************/
/* bootd - boot default image */
/*******************************************************************/
#if defined(CONFIG_CMD_BOOTD)
int do_bootd(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return run_command(env_get("bootcmd"), flag);
}

U_BOOT_CMD(
	boot,	1,	1,	do_bootd,
	"boot default, i.e., run 'bootcmd'",
	""
);

/* keep old command name "bootd" for backward compatibility */
U_BOOT_CMD(
	bootd, 1,	1,	do_bootd,
	"boot default, i.e., run 'bootcmd'",
	""
);

#endif


/*******************************************************************/
/* iminfo - print header info for a requested image */
/*******************************************************************/
#if defined(CONFIG_CMD_IMI)
static int do_iminfo(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int	arg;
	ulong	addr;
	int	rcode = 0;

	if (argc < 2) {
		return image_info(load_addr);
	}

	for (arg = 1; arg < argc; ++arg) {
		addr = simple_strtoul(argv[arg], NULL, 16);
		if (image_info(addr) != 0)
			rcode = 1;
	}
	return rcode;
}

static int image_info(ulong addr)
{
	void *hdr = (void *)addr;

	printf("\n## Checking Image at %08lx ...\n", addr);

	switch (genimg_get_format(hdr)) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
	case IMAGE_FORMAT_LEGACY:
		puts("   Legacy image found\n");
		if (!image_check_magic(hdr)) {
			puts("   Bad Magic Number\n");
			return 1;
		}

		if (!image_check_hcrc(hdr)) {
			puts("   Bad Header Checksum\n");
			return 1;
		}

		image_print_contents(hdr);

		puts("   Verifying Checksum ... ");
		if (!image_check_dcrc(hdr)) {
			puts("   Bad Data CRC\n");
			return 1;
		}
		puts("OK\n");
		return 0;
#endif
#if defined(CONFIG_ANDROID_BOOT_IMAGE)
	case IMAGE_FORMAT_ANDROID:
		puts("   Android image found\n");
		android_print_contents(hdr);
		return 0;
#endif
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		puts("   FIT image found\n");

		if (fit_check_format(hdr, IMAGE_SIZE_INVAL)) {
			puts("Bad FIT image format!\n");
			return 1;
		}

		fit_print_contents(hdr);

		if (!fit_all_image_verify(hdr)) {
			puts("Bad hash in FIT image!\n");
			return 1;
		}

		return 0;
#endif
	default:
		puts("Unknown image format!\n");
		break;
	}

	return 1;
}

U_BOOT_CMD(
	iminfo,	CONFIG_SYS_MAXARGS,	1,	do_iminfo,
	"print header information for application image",
	"addr [addr ...]\n"
	"    - print header information for application image starting at\n"
	"      address 'addr' in memory; this includes verification of the\n"
	"      image contents (magic number, header and payload checksums)"
);
#endif


/*******************************************************************/
/* imls - list all images found in flash */
/*******************************************************************/
#if defined(CONFIG_CMD_IMLS)
static int do_imls_nor(void)
{
	flash_info_t *info;
	int i, j;
	void *hdr;

	for (i = 0, info = &flash_info[0];
		i < CONFIG_SYS_MAX_FLASH_BANKS; ++i, ++info) {

		if (info->flash_id == FLASH_UNKNOWN)
			goto next_bank;
		for (j = 0; j < info->sector_count; ++j) {

			hdr = (void *)info->start[j];
			if (!hdr)
				goto next_sector;

			switch (genimg_get_format(hdr)) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
			case IMAGE_FORMAT_LEGACY:
				if (!image_check_hcrc(hdr))
					goto next_sector;

				printf("Legacy Image at %08lX:\n", (ulong)hdr);
				image_print_contents(hdr);

				puts("   Verifying Checksum ... ");
				if (!image_check_dcrc(hdr)) {
					puts("Bad Data CRC\n");
				} else {
					puts("OK\n");
				}
				break;
#endif
#if defined(CONFIG_FIT)
			case IMAGE_FORMAT_FIT:
				if (fit_check_format(hdr, IMAGE_SIZE_INVAL))
					goto next_sector;

				printf("FIT Image at %08lX:\n", (ulong)hdr);
				fit_print_contents(hdr);
				break;
#endif
			default:
				goto next_sector;
			}

next_sector:		;
		}
next_bank:	;
	}
	return 0;
}
#endif

#if defined(CONFIG_CMD_IMLS_NAND)
static int nand_imls_legacyimage(struct mtd_info *mtd, int nand_dev,
				 loff_t off, size_t len)
{
	void *imgdata;
	int ret;

	imgdata = malloc(len);
	if (!imgdata) {
		printf("May be a Legacy Image at NAND device %d offset %08llX:\n",
				nand_dev, off);
		printf("   Low memory(cannot allocate memory for image)\n");
		return -ENOMEM;
	}

	ret = nand_read_skip_bad(mtd, off, &len, NULL, mtd->size, imgdata);
	if (ret < 0 && ret != -EUCLEAN) {
		free(imgdata);
		return ret;
	}

	if (!image_check_hcrc(imgdata)) {
		free(imgdata);
		return 0;
	}

	printf("Legacy Image at NAND device %d offset %08llX:\n",
			nand_dev, off);
	image_print_contents(imgdata);

	puts("   Verifying Checksum ... ");
	if (!image_check_dcrc(imgdata))
		puts("Bad Data CRC\n");
	else
		puts("OK\n");

	free(imgdata);

	return 0;
}

static int nand_imls_fitimage(struct mtd_info *mtd, int nand_dev, loff_t off,
			      size_t len)
{
	void *imgdata;
	int ret;

	imgdata = malloc(len);
	if (!imgdata) {
		printf("May be a FIT Image at NAND device %d offset %08llX:\n",
				nand_dev, off);
		printf("   Low memory(cannot allocate memory for image)\n");
		return -ENOMEM;
	}

	ret = nand_read_skip_bad(mtd, off, &len, NULL, mtd->size, imgdata);
	if (ret < 0 && ret != -EUCLEAN) {
		free(imgdata);
		return ret;
	}

	if (fit_check_format(imgdata, IMAGE_SIZE_INVAL)) {
		free(imgdata);
		return 0;
	}

	printf("FIT Image at NAND device %d offset %08llX:\n", nand_dev, off);

	fit_print_contents(imgdata);
	free(imgdata);

	return 0;
}

static int do_imls_nand(void)
{
	struct mtd_info *mtd;
	int nand_dev = nand_curr_device;
	size_t len;
	loff_t off;
	u32 buffer[16];

	if (nand_dev < 0 || nand_dev >= CONFIG_SYS_MAX_NAND_DEVICE) {
		puts("\nNo NAND devices available\n");
		return -ENODEV;
	}

	printf("\n");

	for (nand_dev = 0; nand_dev < CONFIG_SYS_MAX_NAND_DEVICE; nand_dev++) {
		mtd = get_nand_dev_by_index(nand_dev);
		if (!mtd->name || !mtd->size)
			continue;

		for (off = 0; off < mtd->size; off += mtd->erasesize) {
			const image_header_t *header;
			int ret;

			if (nand_block_isbad(mtd, off))
				continue;

			len = sizeof(buffer);

			ret = nand_read(mtd, off, &len, (u8 *)buffer);
			if (ret < 0 && ret != -EUCLEAN) {
				printf("NAND read error %d at offset %08llX\n",
						ret, off);
				continue;
			}

			switch (genimg_get_format(buffer)) {
#if defined(CONFIG_IMAGE_FORMAT_LEGACY)
			case IMAGE_FORMAT_LEGACY:
				header = (const image_header_t *)buffer;

				len = image_get_image_size(header);
				nand_imls_legacyimage(mtd, nand_dev, off, len);
				break;
#endif
#if defined(CONFIG_FIT)
			case IMAGE_FORMAT_FIT:
				len = fit_get_size(buffer);
				nand_imls_fitimage(mtd, nand_dev, off, len);
				break;
#endif
			}
		}
	}

	return 0;
}
#endif

#if defined(CONFIG_CMD_IMLS) || defined(CONFIG_CMD_IMLS_NAND)
static int do_imls(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int ret_nor = 0, ret_nand = 0;

#if defined(CONFIG_CMD_IMLS)
	ret_nor = do_imls_nor();
#endif

#if defined(CONFIG_CMD_IMLS_NAND)
	ret_nand = do_imls_nand();
#endif

	if (ret_nor)
		return ret_nor;

	if (ret_nand)
		return ret_nand;

	return (0);
}

U_BOOT_CMD(
	imls,	1,		1,	do_imls,
	"list all images found in flash",
	"\n"
	"    - Prints information about all images found at sector/block\n"
	"      boundaries in nor/nand flash."
);
#endif
