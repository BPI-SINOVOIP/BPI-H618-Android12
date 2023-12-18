/*
 * Copyright (C) 2016 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI SY8827G  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __SY8827G_H__
#define __SY8827G_H__

#define SY8827G_CHIP_ID             (0x80)

#define SY8827G_DEVICE_ADDR			(0x3A3)
#ifdef CONFIG_SY8827G_SUNXI_I2C_SLAVE
#define SY8827G_RUNTIME_ADDR		CONFIG_SY8827G_SUNXI_I2C_SLAVE
#else
#define SY8827G_RUNTIME_ADDR        (0x60)
#endif

/* List of registers for sy8827g */
#define SY8827G_VSEL0		0x00
#define SY8827G_VSEL1		0x01
#define SY8827G_CTRL		0x02
#define SY8827G_ID1		    0x03
#define SY8827G_ID2		    0x04
#define SY8827G_PGOOD		0x05

#define SY8827G_DCDC_PWM_BIT				(6)

int pmu_sy8827g_probe(void);
int pmu_sy8827g_set_dcdc_mode(const char *name, int mode);
int tcs_set_dcdc_mode(void);
bool get_sy8827g(void);
#endif /* __SY8827G_REGS_H__ */


