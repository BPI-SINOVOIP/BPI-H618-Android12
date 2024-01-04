/*
 * (C) Copyright 2007-2013
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Jerry Wang <wangflord@allwinnertech.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef  __SUNXI_BOOT_SPI_H__
#define  __SUNXI_BOOT_SPI_H__

#include "spare_head.h"

#define BOOT0_SIZE_BYTES			(32*1024)
#define BOOT1_SIZE_BYTES			(8*1024*1024)


extern int getFlashType(void);
extern int check_soc_is_secure(void);
extern void clearPageCache (void);
extern int sunxi_read_uboot(BufferExtractCookie* cookie, int flash_type);
extern int sunxi_write_uboot(BufferExtractCookie* cookie, int flash_type, int generate_checksum);
extern int sunxi_read_boot0(BufferExtractCookie* cookie, int flash_type);
extern int sunxi_write_boot0(BufferExtractCookie* cookie, int flash_type, int generate_checksum);

extern uint generate_check_sum(void *mem_base, uint length, uint src_sum);

/*
 * implement this interface to modify the data
 */
typedef int (*parameter_callback)(void *, void *);
typedef int (*item_callback)(void *, void *, void *);
int libboot_read_item(parameter_callback cb, void *out, void *item_name);
int libboot_update_item(parameter_callback cb, void *in, void *item_name);
int libboot_read_boot(item_callback cb, void *out, int boot_num, void *item_name);
int libboot_update_boot(item_callback cb, void *in, int boot_num, void *item_name);
int libboot_burn_boot(item_callback cb, void *in, int boot_num, void *item);

extern int sunxi_verify_toc0(void* buf);
extern int sunxi_verify_toc1(void* buf);

#endif
