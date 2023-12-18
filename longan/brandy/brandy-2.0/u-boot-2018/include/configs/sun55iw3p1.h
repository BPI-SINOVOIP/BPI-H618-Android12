/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Configuration settings for the Allwinner A64 (sun50i) CPU
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * A64 specific configuration
 */
#define SUNXI_ARM_A53

#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_USB_EHCI_SUNXI
#define CONFIG_USB_MAX_CONTROLLER_COUNT 2
#endif

#define CONFIG_SUNXI_USB_PHYS	1

#define GICD_BASE		0x3021000
#define GICC_BASE		0x3022000

/* sram layout*/
/*#define SUNXI_SRAM_A2_BASE	(SUNXI_SRAM_A2_BASE)*/

#define SUNXI_SRAM_A2_SIZE		(0x24000 - 0x4000)

#define SUNXI_SYS_SRAM_BASE		SUNXI_SRAM_A2_BASE + 0x4000
#define SUNXI_SYS_SRAM_SIZE		SUNXI_SRAM_A2_SIZE

#define CONFIG_SYS_BOOTM_LEN 0x3000000

#define PHOENIX_PRIV_DATA_ADDR      (0x61500)                      //给phoenix保留的空间

/*
 * Include common sunxi configuration where most the settings are
 */
#include <configs/sunxi-common.h>

#endif /* __CONFIG_H */
