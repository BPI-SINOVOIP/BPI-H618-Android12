/*
 * (C) Copyright 2018-2020
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 *
 * Some init for sunxi platform.
 */

#ifndef _BOARD_HELPER_H_
#define _BOARD_HELPER_H_

int sunxi_update_fdt_para_for_kernel(void);
char get_switch_flag(void);
char get_switch_kernel_flag(void);
char get_switch_rootfs_flag(void);

int fdt_enable_node(char *name, int onoff);

#endif /*_BOARD_HELPER_H_*/
