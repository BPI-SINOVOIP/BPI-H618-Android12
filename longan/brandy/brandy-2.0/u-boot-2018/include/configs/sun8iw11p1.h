/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2014 Chen-Yu Tsai <wens@csie.org>
 *
 * Configuration settings for the Allwinner A23 (sun8i) CPU
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * Include common sunxi configuration where most the settings are
 */
#include <configs/sunxi-common.h>

#define SUNXI_DMA_SECURITY
/*
 * sun8iw18 specific configuration
 */
/*#define CONFIG_SF_DEFAULT_BUS   0
#define CONFIG_SF_DEFAULT_CS    0
#define CONFIG_SF_DEFAULT_SPEED 50000000
#define CONFIG_SF_DEFAULT_MODE  (SPI_RX_DUAL|SPI_RX_QUAD)
*/
#ifdef CONFIG_USB_EHCI_HCD
#define CONFIG_USB_EHCI_SUNXI
#define CONFIG_USB_MAX_CONTROLLER_COUNT 3
#endif

/* sram layout*/
#define SUNXI_SRAM_A1_BASE		(0x0)
#define SUNXI_SRAM_A1_SIZE		(0x08000)

#define SUNXI_SYS_SRAM_BASE		SUNXI_SRAM_A1_BASE
#define SUNXI_SYS_SRAM_SIZE		SUNXI_SRAM_A1_SIZE


#define CONFIG_SUNXI_USB_PHYS	2

/* twi0 use the same pin with uart, the default use twi1 */
#define CONFIG_SYS_SPD_BUS_NUM	0

#define IR_BASE SUNXI_R_CIR_RX_BASE

/*undefine coms that sun8iw18 doesnt have*/
#undef CONFIG_SYS_NS16550_COM5
#define CONFIG_SYS_BOOTM_LEN 0x2000000
#endif /* __CONFIG_H */
