// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2023-2026
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

int get_group_bit_offset(enum pin_e port_group)
{
	switch (port_group) {
	case GPIO_GROUP_A:
	case GPIO_GROUP_C:
	case GPIO_GROUP_D:
	case GPIO_GROUP_E:
	case GPIO_GROUP_F:
	case GPIO_GROUP_G:
	case GPIO_GROUP_I:
		return port_group;
		break;
	default:
		return -1;
	}
    return -1;
}

