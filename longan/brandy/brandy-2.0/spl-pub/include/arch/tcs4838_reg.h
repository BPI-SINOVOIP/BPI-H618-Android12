/*
 * Copyright (C) 2016 Allwinner.
 * wangwei <wangwei@allwinnertech.com>
 *
 * SUNXI AXP21  Driver
 *
 */

#ifndef __TCS4838_H__
#define __TCS4838_H__

#include <arch/axp.h>

#define TCS4838_CHIP_ID             (0x80)

#ifdef CFG_SUNXI_TWI
#define TCS4838_DEVICE_ADDR		400000
#define TCS4838_RUNTIME_ADDR		(0x41)
#else
#define TCS4838_DEVICE_ADDR		(0x3A3)
#define TCS4838_RUNTIME_ADDR		(0x41)
#endif

/* List of registers for tcs4838 */
#define TCS4838_VSEL0		0x00
#define TCS4838_VSEL1		0x01
#define TCS4838_CTRL		0x02
#define TCS4838_ID1		    0x03
#define TCS4838_ID2		    0x04
#define TCS4838_TCS_PGOOD	0x05

int tcs4838_set_pll_voltage(int set_vol);
int tcs4838_tcs_init(u8 power_mode);

typedef struct _tcs_step_info {
	int step_min_vol;
	int step_max_vol;
	int step_val;
	u32 regation;
} _tcs_step_info;

typedef struct _tcs_contrl_info {
	char name[16];

	int min_vol;
	int max_vol;
	u32 cfg_reg_addr;
	u32 cfg_reg_mask;
	u32 ctrl_reg_addr;
	u32 ctrl_bit_ofs;
	u32 reg_addr_offest;
	_tcs_step_info tcs_step_tbl[4];

} tcs_contrl_info;

#endif /* __TCS4838_REGS_H__ */

