/*
 * Copyright (C) 2019 Allwinner.
 * weidonghui <weidonghui@allwinnertech.com>
 *
 * SUNXI AXP  Driver
 *
 * SPDX-License-Identifier: GPL-2.0+
 */

#ifndef __PMU_EXT_H__
#define __PMU_EXT_H__

#include <common.h>
#include <linker_lists.h>

struct sunxi_pmu_ext_dev_t {
const char *pmu_ext_name;
int (*probe)(void); /* matches chipid*/
bool (*get_exist)(void); /*get axp info*/
int (*set_dcdc_mode)(const char *name, int mode); /*force dcdc mode in pwm or not */
};

enum {
	TCS4838 = 0,
	SY8827G,
	AXP1530,
	NR_PMU_EXT_VARIANTS,
};

#define U_BOOT_PMU_EXT_INIT(_name)                                             \
	ll_entry_declare(struct sunxi_pmu_ext_dev_t, _name, pmu_ext)

int pmu_ext_probe(void);
bool pmu_ext_get_exist(void);
int pmu_set_dcdc_mode_ext(const char *name, int mode);
int pmu_ext_get_type(void);

#endif /* __PMU_EXT_H__ */
