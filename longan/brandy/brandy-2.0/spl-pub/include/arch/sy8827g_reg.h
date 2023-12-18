/*
 * Copyright (C) 2016 Allwinner.
 * wangwei <wangwei@allwinnertech.com>
 *
 * SUNXI AXP21  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __SY8827G_H__
#define __SY8827G_H__

#include <arch/axp.h>

#define SY8827G_CHIP_ID             (0x80)

#ifdef CFG_SUNXI_TWI
#define SY8827G_DEVICE_ADDR		400000
#define SY8827G_RUNTIME_ADDR		(0x60)
#else
#define SY8827G_DEVICE_ADDR		(0x3A3)
#define SY8827G_RUNTIME_ADDR		(0x60)
#endif

/* List of registers for sy8827g */
#define SY8827G_VSEL0		0x00
#define SY8827G_VSEL1		0x01
#define SY8827G_CTRL		0x02
#define SY8827G_ID1		    0x03
#define SY8827G_ID2		    0x04
#define SY8827G_TCS_PGOOD	0x05

int sy8827g_set_pll_voltage(int set_vol);
int sy8827g_ext_init(u8 power_mode);

typedef struct _ext_step_info {
	int step_min_vol;
	int step_max_vol;
	int step_val;
	u32 regation;
} _ext_step_info;

typedef struct _ext_contrl_info {
	char name[16];

	int min_vol;
	int max_vol;
	u32 cfg_reg_addr;
	u32 cfg_reg_mask;
	u32 ctrl_reg_addr;
	u32 ctrl_bit_ofs;
	u32 reg_addr_offest;
	_ext_step_info ext_step_tbl[4];

} ext_contrl_info;

#endif /* __SY8827G_REGS_H__ */

