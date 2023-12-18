// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2022
 * allwinner Software Engineering,.
 */

#include <common.h>
#include <serial.h>
#include "private_uboot.h"
struct serial_device *default_serial_console(void)
{
	switch (uboot_spare_head.boot_data.uart_port) {
#if defined(CONFIG_SYS_NS16550_COM2)
	case 1:
		return &eserial2_device;
#endif
#if defined(CONFIG_SYS_NS16550_COM3)
	case 2:
		return &eserial3_device;
#endif
#if defined(CONFIG_SYS_NS16550_COM4)
	case 3:
		return &eserial4_device;
#endif
#if defined(CONFIG_SYS_NS16550_COM5)
	case 4:
		return &eserial5_device;
#endif
#if defined(CONFIG_SYS_NS16550_COM6)
	case 5:
		return &eserial6_device;
#endif

#if defined(CONFIG_SYS_NS16550_COM1)
	default:
		return &eserial1_device;
#endif
	}
}
