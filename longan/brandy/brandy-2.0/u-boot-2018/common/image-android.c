// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 */

#include <common.h>
#include <image.h>
#include <android_image.h>
#include <malloc.h>
#include <errno.h>
#include <sys_partition.h>
#include <sunxi_flash.h>
#include <fdt_support.h>
#include <libxbc.h>
#include <sunxi_avb.h>

#define ANDROID_IMAGE_DEFAULT_KERNEL_ADDR	0x10008000
int do_sunxi_flash(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[]);
struct boot_img_hdr *get_init_boot_hdr_addr(const struct andr_img_hdr *hdr,
					    uint part_offset, uint part_size);
static char andr_tmp_str[ANDR_BOOT_ARGS_SIZE + 1];

static struct vendor_boot_img_hdr *vendor_hdr;
static struct boot_img_hdr *init_boot_hdr;

/* static char img_cmdline[ANDR_BOOT_ARGS_SIZE + ANDR_BOOT_EXTRA_ARGS_SIZE]; */
static char *img_cmdline;

int android_image_is_vendor_bootconfig_used(
	const struct vendor_boot_img_hdr *vendor_hdr)
{
	return vendor_hdr->vendor_bootconfig_size > 0;
}

int android_image_get_vendor_bootconfig(const struct andr_img_hdr *hdr,
					ulong *rd_data, ulong *rd_len)
{
	ulong bconfig_start;
	ulong bconfig_len;
	if ((!strncmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) &&
	    (hdr->unused >= 0x3)) {
		vendor_hdr = get_vendor_hdr_addr();
		if (vendor_hdr == 0) {
			pr_err("err: vendor_hdr=0\n");
			return -1;
		}
		if (!android_image_is_vendor_bootconfig_used(vendor_hdr)) {
			pr_err("vendor bootconfig is not use");
			return -1;
		}
		bconfig_start = (unsigned long)vendor_hdr;
		bconfig_start +=
			ALIGN(VENDOR_BOOT_HEAD_SIZE, vendor_hdr->page_size) +
			ALIGN(vendor_hdr->vendor_ramdisk_size,
			      vendor_hdr->page_size) +
			ALIGN(vendor_hdr->dtb_size, vendor_hdr->page_size) +
			ALIGN(vendor_hdr->vendor_ramdisk_table_size,
			      vendor_hdr->page_size);

		bconfig_len = vendor_hdr->vendor_bootconfig_size;
		bconfig_len += addBootConfigTrailer(bconfig_start, bconfig_len);
		*rd_data = bconfig_start;
		*rd_len	 = bconfig_len;
		return 0;
	}
	return -1;
}

int android_image_get_vendor_get_end(const struct vendor_boot_img_hdr *vendor_hdr)
{
	uint end = -1;
	if (strncmp((const char *)vendor_hdr->magic, VENDOR_BOOT_MAGIC, VENDOR_BOOT_MAGIC_SIZE)) {
		pr_err("vendor magic err\n");
		return -1;
	}
	end = (unsigned long)vendor_hdr;
	end += ALIGN(VENDOR_BOOT_HEAD_SIZE, vendor_hdr->page_size);
	end += ALIGN(vendor_hdr->vendor_ramdisk_size, vendor_hdr->page_size);
	end += ALIGN(vendor_hdr->dtb_size, vendor_hdr->page_size);

	if (android_image_is_vendor_bootconfig_used(vendor_hdr)) {
		end += ALIGN(vendor_hdr->vendor_ramdisk_table_size,
			     vendor_hdr->page_size);
		end += ALIGN(vendor_hdr->vendor_bootconfig_size,
			     vendor_hdr->page_size);
	}

	/* tick_printf("vendor end:0x%x\n", end); */
	return end;
}

struct vendor_boot_img_hdr *get_vendor_hdr_addr(void)
{
	static int already_vendor;
	uint part_offset, part_size;
	if (!already_vendor) {
		already_vendor = 1;
		if (sunxi_partition_get_info_byname("vendor_boot", &part_offset, &part_size)) {
			pr_err("no %s partition is found\n", "vendor_boot");
			return NULL;
		} else {
#ifdef CONFIG_SUNXI_VENDOR_BOOT_ADDR
			sunxi_flash_read(part_offset, ALIGN(VENDOR_BOOT_HEAD_SIZE, 2048)/512, (char *)CONFIG_SUNXI_VENDOR_BOOT_ADDR);
			part_size = android_image_get_vendor_get_end((const struct vendor_boot_img_hdr *)CONFIG_SUNXI_VENDOR_BOOT_ADDR);
			if (part_size > 0) {
				/* part_size = vendor_boot_end - vendor_boot_start*/

				part_size = ALIGN(part_size - CONFIG_SUNXI_VENDOR_BOOT_ADDR, 512)/512;
				/* pr_err("part_offset:0x%x, part_size:0x%x\n", part_offset, part_size); */
				sunxi_flash_read(part_offset, part_size, (char *)CONFIG_SUNXI_VENDOR_BOOT_ADDR);
				vendor_hdr = (vendor_boot_img_hdr *)CONFIG_SUNXI_VENDOR_BOOT_ADDR;
			} else {
				vendor_hdr = NULL;
			}
#else
			return NULL;
#endif
		}
	}
	return vendor_hdr;
}

#ifdef CONFIG_SUNXI_ANDROID_OVERLAY
int dtbo_idx[10];
int strtoint(char *str)
{
	char *p = str;
	int val = 0;
	while (*p != '\0') {
		val = val * 10 + (*p - '0');
		p++;
	}
	return val;
}
void get_andriod_dtbo_idx(const char *cmdline, const char *name)
{
	char value[10] = {0};
	char *p = NULL;
	int i = 0;
	int count = 0;

	if (cmdline == NULL) {
		pr_msg("%s cmdline is NULL", __func__);
		return;
	}
	memset((void *)dtbo_idx, 0xf5, sizeof(dtbo_idx));
	p = (char *)cmdline;
	/* compare if it is the last character int the string*/
	while (*p != '\0') {
		/*compare if the end of each key*/
		if ((*p == ' ') || (*p == 0xa)) {
			/* Find the address of next key in the string */
			p++;
			/* Search for the specified key:name */
			if (!strncmp(p, name, strlen(name))) {
				p += strlen(name);

				/* Find the address offset of the value of key:name  in the string */
				if (*p++ != '=') {
					continue;
				}

				/* compare if it is the end of key:name */
				while ((*p != '\0') && (*p != ' ') &&
				       (*p != 0xa)) {
					if (*p != ',') {
						value[i] = *p;
						i++;
					} else {
						value[i] = '\0';
						i	= 0;
						/*save vaule to dtbo_idx*/
						dtbo_idx[count] =
							strtoint(value);
						pr_msg("line:%d dtbo_idx= %d\n",
						       __LINE__,
						       dtbo_idx[count]);
						count++;
					}
					p++;
				}
				value[i]	= '\0';
				i		= 0;
				dtbo_idx[count] = strtoint(value);
				pr_msg("dtbo_idx= %d\n", __LINE__,
				       dtbo_idx[count]);
				return;
			}
		} else {
			p++;
		}
	}
	pr_msg("%s  can't find", name);
	return;
}
#endif

static ulong android_image_get_kernel_addr(const struct andr_img_hdr *hdr)
{
	/*
	 * All the Android tools that generate a boot.img use this
	 * address as the default.
	 *
	 * Even though it doesn't really make a lot of sense, and it
	 * might be valid on some platforms, we treat that adress as
	 * the default value for this field, and try to execute the
	 * kernel in place in such a case.
	 *
	 * Otherwise, we will return the actual value set by the user.
	 */
	if ((!strncmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) && (hdr->unused >= 0x3)) {
		vendor_hdr = get_vendor_hdr_addr();
		if (vendor_hdr != NULL) {
			return vendor_hdr->kernel_addr;
		} else {
			return 0;
		}
	} else {
		if (hdr->kernel_addr == ANDROID_IMAGE_DEFAULT_KERNEL_ADDR)
			return (ulong)hdr + hdr->page_size;

		return hdr->kernel_addr;
	}
}

/**
 * android_image_get_kernel() - processes kernel part of Android boot images
 * @hdr:	Pointer to image header, which is at the start
 *			of the image.
 * @verify:	Checksum verification flag. Currently unimplemented.
 * @os_data:	Pointer to a ulong variable, will hold os data start
 *			address.
 * @os_len:	Pointer to a ulong variable, will hold os data length.
 *
 * This function returns the os image's start address and length. Also,
 * it appends the kernel command line to the bootargs env variable.
 *
 * Return: Zero, os start address and length on success,
 *		otherwise on failure.
 */

int android_image_get_kernel(const struct andr_img_hdr *hdr, int verify,
			     ulong *os_data, ulong *os_len)
{
	u32 kernel_addr = android_image_get_kernel_addr(hdr);
	struct boot_img_hdr *hdr_v3;
	u32 page_size = hdr->page_size;
	u32 kernel_size = hdr->kernel_size;
	char *name = (char *)hdr->name;
	img_cmdline = (char *)hdr->cmdline;

	ulong bconfig_start;
	ulong bconfig_len;
	char end_flag = 0;

	if ((!strncmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) && (hdr->unused >= 0x3)) {
		vendor_hdr = get_vendor_hdr_addr();
		hdr_v3 = (boot_img_hdr *)hdr;
		if (android_image_is_vendor_bootconfig_used(vendor_hdr)) {
			android_image_get_vendor_bootconfig(hdr, &bconfig_start,
							    &bconfig_len);
			img_cmdline = (char *)bconfig_start;
			end_flag    = 0xa;
		} else {
			img_cmdline = (char *)vendor_hdr->cmdline;
			end_flag    = ' ';
		}
		if (vendor_hdr != NULL) {
			page_size = vendor_hdr->page_size*2;
			name = (char *)vendor_hdr->name;
		}
		kernel_size = ALIGN(hdr_v3->kernel_size, page_size);
	}
	/*
	 * Not all Android tools use the id field for signing the image with
	 * sha1 (or anything) so we don't check it. It is not obvious that the
	 * string is null terminated so we take care of this.
	 */
#ifdef CONFIG_SUNXI_ANDROID_OVERLAY
	char *dtboidx = env_get("dtbo_idx");
	/* if env set dtbo_idx, replace image dtbo_idx, and update cmdline. */
	/*
	 * |...            dtbo_idx=${old_value} ...|
	 * v                        v           v
	 * |...                     idx         p ...|
	 */
	static char image_cmdline[512] = {0};
	if (dtboidx != NULL) {
		char *p = NULL;
		int idx = 0;
		strncpy(image_cmdline, img_cmdline, strlen(img_cmdline));
		p = img_cmdline;
		/* compare if it is the last character int the string*/
		do {
			/* Search for the specified key: androidboot.dtbo_idx */
			if (!strncmp(p, ANDR_BOOT_DTBO_MAIGC, strlen(ANDR_BOOT_DTBO_MAIGC))) {
				p += strlen(ANDR_BOOT_DTBO_MAIGC);
				/* Find the address offset of the value of androidboot.dtbo_idx key in the string */
				if (*p++ == '=') {
					idx = p - img_cmdline;
					/* Find the address offset of the next key of the androidboot.dtbo_idx */
					while (*p != '\0' && *p != end_flag)
						p++;
					break;
				}
			}

			/* Find the address offset of the first character of each key in the string */
			while (*p++ != end_flag)
				;
		} while (*p != '\0' && idx == 0);

		/*compare whether the androidboot.dtbo_idx key exists */
		if (idx != 0) {
			image_cmdline[idx] = '\0';
			strcat(image_cmdline, dtboidx);
			strcat(image_cmdline, p);
		}
		img_cmdline = image_cmdline;
	}
	get_andriod_dtbo_idx(img_cmdline, ANDR_BOOT_DTBO_MAIGC);
#endif
	if (end_flag == 0xa) {
		img_cmdline = (char *)vendor_hdr->cmdline;
	}
	strncpy(andr_tmp_str, name, ANDR_BOOT_NAME_SIZE);
	andr_tmp_str[ANDR_BOOT_NAME_SIZE] = '\0';
	if (strlen(andr_tmp_str))
		printf("Android's image name: %s\n", andr_tmp_str);

	debug("Kernel load addr 0x%08x size %u KiB\n",
	       kernel_addr, DIV_ROUND_UP(kernel_size, 1024));

	int len = 0;
	if (*img_cmdline) {
		debug("Kernel command line: %s\n", img_cmdline);
		len += strlen(img_cmdline);
	}

	char *bootargs = env_get("bootargs");
	if (bootargs)
		len += strlen(bootargs);

	char *newbootargs = malloc(len + 2);
	if (!newbootargs) {
		puts("Error: malloc in android_image_get_kernel failed!\n");
		return -ENOMEM;
	}
	*newbootargs = '\0';

	if (bootargs) {
		strcpy(newbootargs, bootargs);
		strcat(newbootargs, " ");
	}
	if (*img_cmdline)
		strcat(newbootargs, img_cmdline);

	env_set("bootargs", newbootargs);

	if (os_data) {
		*os_data = (ulong)hdr;
		*os_data += page_size;
	}
	if (os_len)
		*os_len = kernel_size;
	if (newbootargs)
		free(newbootargs);
	return 0;
}

int android_image_check_header(const struct andr_img_hdr *hdr)
{
	return memcmp(ANDR_BOOT_MAGIC, hdr->magic, ANDR_BOOT_MAGIC_SIZE);
}

ulong android_image_get_end_by_avbfooter(void)
{
#ifdef CONFIG_SUNXI_AVB
	u32 start_block;
	u32 part_sectors;
	AvbFooter footer;
	uint8_t *avb_footer_buf;
	uint8_t avb_footer_sector[512];
	char *part_name = env_get("boot_from_partion");
	if (!part_name)
		return 0;
	if (sunxi_partition_get_info_byname(part_name, &start_block,
					    &part_sectors)) {
		return 0;
	}
	sunxi_flash_read(start_block + part_sectors - 1, 1, avb_footer_sector);
	avb_footer_buf = &avb_footer_sector[512 - AVB_FOOTER_SIZE];
	if (!avb_footer_validate_and_byteswap((const AvbFooter *)avb_footer_buf,
					      &footer)) {
		return 0;
	}
	return round_down(footer.vbmeta_offset, 512);
#else
	return 0;
#endif
}

ulong android_image_get_end(const struct andr_img_hdr *hdr)
{
	ulong end;
	struct boot_img_hdr *hdr_v3;
	/*
	 * The header takes a full page, the remaining components are aligned
	 * on page boundary
	 */
	end = (ulong)hdr;
	if ((!strncmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) && (hdr->unused >= 0x3)) {
		vendor_hdr = get_vendor_hdr_addr();
		if (vendor_hdr != NULL) {
			hdr_v3 = (boot_img_hdr *)hdr;
			end += ALIGN(hdr_v3->header_size, vendor_hdr->page_size*2);
			end += ALIGN(hdr_v3->kernel_size, vendor_hdr->page_size*2);
			end += ALIGN(hdr_v3->ramdisk_size, vendor_hdr->page_size*2);
		}
		debug("hdr_v3 end:0x%x\n", (uint)end);
	} else {
		end += hdr->page_size;
		end += ALIGN(hdr->kernel_size, hdr->page_size);
		end += ALIGN(hdr->ramdisk_size, hdr->page_size);
		end += ALIGN(hdr->second_size, hdr->page_size);
		end += ALIGN(hdr->recovery_dtbo_size, hdr->page_size);
		end += ALIGN(hdr->dtb_size, hdr->page_size);
	}

	return end;
}

ulong android_image_get_kload(const struct andr_img_hdr *hdr)
{
	return android_image_get_kernel_addr(hdr);
}

int android_image_get_ramdisk(const struct andr_img_hdr *hdr,
			      ulong *rd_data, ulong *rd_len)
{
	struct boot_img_hdr *hdr_v3;
	u32 page_size	 = hdr->page_size;
	u32 kernel_size	 = hdr->kernel_size;
	u32 ramdisk_size = hdr->ramdisk_size;
	u32 ramdisk_addr = hdr->ramdisk_addr;
	uint part_offset, part_size;
	struct boot_img_hdr *init_boot = NULL;

	if (!sunxi_partition_get_info_byname("init_boot", &part_offset,
					     &part_size)) {
		printf("ramdisk use init boot\n");
		init_boot = get_init_boot_hdr_addr(
			hdr, part_offset, part_size);
		hdr = (struct andr_img_hdr *)init_boot;
	}

	if ((!strncmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) &&
	    (hdr->unused >= 0x3)) {
		vendor_hdr  = get_vendor_hdr_addr();
		hdr_v3      = (boot_img_hdr *)hdr;
		kernel_size = hdr_v3->kernel_size;
		if (vendor_hdr != 0) {
			page_size    = vendor_hdr->page_size * 2;
			ramdisk_addr = vendor_hdr->ramdisk_addr;
		}
		ramdisk_size = hdr_v3->ramdisk_size;
	}
	if (!ramdisk_size) {
		*rd_data = *rd_len = 0;
		return -1;
	}

	debug("RAM disk load addr 0x%08x size %u KiB\n", ramdisk_addr,
	      DIV_ROUND_UP(ramdisk_size, 1024));

	*rd_data = (unsigned long)hdr;
	*rd_data += page_size;
	*rd_data += ALIGN(kernel_size, page_size);
	*rd_len = ramdisk_size;
	debug("rd_data:0x%lx, rd_len:0x%lx\n", *rd_data, *rd_len);
	return 0;
}

struct boot_img_hdr *get_init_boot_hdr_addr(const struct andr_img_hdr *hdr,
					    uint part_offset, uint part_size)
{
	static int already_initboot;
	ulong initboot_start;
	ulong initboot_end;
	ulong vendor_end;
	//struct vendor_boot_img_hdr *vendor_hdr = NULL;
	if (!already_initboot) {
		already_initboot = 1;
		if (part_size > 0) {
			vendor_hdr = get_vendor_hdr_addr();
			vendor_end =
				android_image_get_vendor_get_end(vendor_hdr);
			if (vendor_end > 0) {
				initboot_start = ALIGN(vendor_end, 2048);
				sunxi_flash_read(part_offset, 2048 / 512,
						 (char *)initboot_start);
				initboot_end = android_image_get_end(
					(void *)initboot_start);
				part_size = ALIGN(initboot_end - initboot_start,
						  512) /
					    512;
				/*printf("hdr:0x%lx, part_offset:0x%x, part_size:0x%x\n",
								(ulong)hdr, part_offset, part_size);*/
				sunxi_flash_read(part_offset, part_size,
						 (char *)initboot_start);
				init_boot_hdr =
					(struct boot_img_hdr *)initboot_start;
			} else {
				return NULL;
			}
		} else {
			return NULL;
		}
	}
	return init_boot_hdr;
}

int android_image_get_vendor_ramdisk(const struct andr_img_hdr *hdr,
				ulong *rd_data, ulong *rd_len)
{
	vendor_hdr = get_vendor_hdr_addr();
	if (vendor_hdr != NULL) {
		if (!vendor_hdr->vendor_ramdisk_size) {
			*rd_data = *rd_len = 0;
			return -1;
		}
		*rd_data = (unsigned long)vendor_hdr;
		*rd_data += ALIGN(VENDOR_BOOT_HEAD_SIZE, vendor_hdr->page_size);

		*rd_len = vendor_hdr->vendor_ramdisk_size;
		debug("rd_data:0x%lx rd_len:0x%lx\n", *rd_data, *rd_len);

	}
	return 0;
}

int android_image_get_second(const struct andr_img_hdr *hdr,
			      ulong *second_data, ulong *second_len)
{
	if (!hdr->second_size) {
		*second_data = *second_len = 0;
		return -1;
	}

	*second_data = (unsigned long)hdr;
	*second_data += hdr->page_size;
	*second_data += ALIGN(hdr->kernel_size, hdr->page_size);
	*second_data += ALIGN(hdr->ramdisk_size, hdr->page_size);

	printf("second address is 0x%lx\n",*second_data);

	*second_len = hdr->second_size;
	return 0;
}

int android_image_get_recovery_dtbo(const struct andr_img_hdr *hdr,
			      ulong *recovery_dtbo_data, ulong *recovery_dtbo_len)
{
	if (!hdr->recovery_dtbo_size) {
		*recovery_dtbo_data = *recovery_dtbo_len = 0;
		return -1;
	}

	*recovery_dtbo_data = (unsigned long)hdr;
	*recovery_dtbo_data += hdr->page_size;
	*recovery_dtbo_data += ALIGN(hdr->kernel_size, hdr->page_size);
	*recovery_dtbo_data += ALIGN(hdr->ramdisk_size, hdr->page_size);
	*recovery_dtbo_data += ALIGN(hdr->second_size, hdr->page_size);

	printf("recovery_dtbo address is 0x%lx\n", *recovery_dtbo_data);

	*recovery_dtbo_len = hdr->recovery_dtbo_size;
	return 0;
}

int android_image_get_dtb(const struct andr_img_hdr *hdr,
			      ulong *dtb_data, ulong *dtb_len)
{
	u32 part_start;
	if (!strncmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) {
		if ((hdr->unused == 0x3)) {
			vendor_hdr = get_vendor_hdr_addr();
			if ((vendor_hdr == NULL) && (!vendor_hdr->dtb_size)) {
				*dtb_data = *dtb_len = 0;
				pr_err("err: dtb_len=0\n");
				return -1;
			}

			*dtb_data = (unsigned long)vendor_hdr;
			*dtb_data += ALIGN(VENDOR_BOOT_HEAD_SIZE, vendor_hdr->page_size);
			*dtb_data += ALIGN(vendor_hdr->vendor_ramdisk_size, vendor_hdr->page_size);
			*dtb_len  = ALIGN(vendor_hdr->dtb_size, vendor_hdr->page_size);
		} else {
			*dtb_data = (unsigned long)hdr;
			*dtb_data += hdr->page_size;
			*dtb_data += ALIGN(hdr->kernel_size, hdr->page_size);
			*dtb_data += ALIGN(hdr->ramdisk_size, hdr->page_size);
			*dtb_data += ALIGN(hdr->second_size, hdr->page_size);
			*dtb_data += ALIGN(hdr->recovery_dtbo_size, hdr->page_size);

			*dtb_len = hdr->dtb_size;
			part_start = sunxi_partition_get_offset_byname("boot");
			if (part_start != 0) {
					sunxi_flash_read(part_start + (*dtb_data - (unsigned long)hdr)/512, ALIGN(hdr->dtb_size, 512)/512, (char *)(*dtb_data));
			}
		}
	}

	debug("dtb address is 0x%lx\n", *dtb_data);
	return fdt_check_header((void *)*dtb_data);
}

#if !defined(CONFIG_SPL_BUILD)
/**
 * android_print_contents - prints out the contents of the Android format image
 * @hdr: pointer to the Android format image header
 *
 * android_print_contents() formats a multi line Android image contents
 * description.
 * The routine prints out Android image properties
 *
 * returns:
 *     no returned results
 */
void android_print_contents(const struct andr_img_hdr *hdr)
{
	const char * const p = IMAGE_INDENT_STRING;
	/* os_version = ver << 11 | lvl */
	u32 os_ver = hdr->os_version >> 11;
	u32 os_lvl = hdr->os_version & ((1U << 11) - 1);

	printf("%skernel size:      %x\n", p, hdr->kernel_size);
	printf("%skernel address:   %x\n", p, hdr->kernel_addr);
	printf("%sramdisk size:     %x\n", p, hdr->ramdisk_size);
	printf("%sramdisk addrress: %x\n", p, hdr->ramdisk_addr);
	printf("%ssecond size:      %x\n", p, hdr->second_size);
	printf("%ssecond address:   %x\n", p, hdr->second_addr);
	printf("%stags address:     %x\n", p, hdr->tags_addr);
	printf("%spage size:        %x\n", p, hdr->page_size);
	printf("%sdtbo size:        %x\n", p, hdr->recovery_dtbo_size);
	printf("%sdtbo address:     %llx\n", p, hdr->recovery_dtbo_offset);
	printf("%sdtb size:         %x\n", p, hdr->dtb_size);
	printf("%sdtb addr:         %llx\n", p, hdr->dtb_addr);
	/* ver = A << 14 | B << 7 | C         (7 bits for each of A, B, C)
	 * lvl = ((Y - 2000) & 127) << 4 | M  (7 bits for Y, 4 bits for M) */
	printf("%sos_version:       %x (ver: %u.%u.%u, level: %u.%u)\n",
	       p, hdr->os_version,
	       (os_ver >> 7) & 0x7F, (os_ver >> 14) & 0x7F, os_ver & 0x7F,
	       (os_lvl >> 4) + 2000, os_lvl & 0x0F);
	printf("%sname:             %s\n", p, hdr->name);
	printf("%scmdline:          %s\n", p, hdr->cmdline);
}
#endif
