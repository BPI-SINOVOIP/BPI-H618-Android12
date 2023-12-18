/*
 * Copyright (C) 2016 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI TCS4838  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __TCS4838_H__
#define __TCS4838_H__

#define TCS4838_CHIP_ID             (0x80)

#define TCS4838_DEVICE_ADDR			(0x3A3)
#ifdef CONFIG_TCS4838_SUNXI_I2C_SLAVE
#define TCS4838_RUNTIME_ADDR		CONFIG_TCS4838_SUNXI_I2C_SLAVE
#else
#define TCS4838_RUNTIME_ADDR        (0x41)
#endif

/* List of registers for tcs4838 */
#define TCS4838_VSEL0		0x00
#define TCS4838_VSEL1		0x01
#define TCS4838_CTRL		0x02
#define TCS4838_ID1		    0x03
#define TCS4838_ID2		    0x04
#define TCS4838_TCS_PGOOD	0x05

#define TCS4838_DCDC_PWM_BIT				(6)

int pmu_tcs4838_probe(void);
int pmu_tcs4838_set_dcdc_mode(const char *name, int mode);
int tcs_set_dcdc_mode(void);
bool get_tcs4838(void);
#endif /* __TCS4838_REGS_H__ */


