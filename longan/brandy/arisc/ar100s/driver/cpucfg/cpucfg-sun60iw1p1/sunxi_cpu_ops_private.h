/*
 * Copyright (c) 2020, Allwinner. All rights reserved.
 *
 * Author: Fan Qinghua <fanqinghua@allwinnertech.com>
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SUNXI_CPU_OPS_PRIVATE_H
#define SUNXI_CPU_OPS_PRIVATE_H

#define BIT(n)							(0x1 << (n))

#define SUNXI_CPU_OPS_BASE				0x07050000
#define CPUS_RST_CTRL_REG				SUNXI_CPU_OPS_BASE
#define HOTPLUG_CONTROL_REG(n)			(SUNXI_CPU_OPS_BASE + 0x200 + (n) * 4)
#define WAKEUP_MASK						BIT(1)
#define HOTPLUG_EN						BIT(0)
#define HOTPLUG_POWERMODE_REG(n)		(SUNXI_CPU_OPS_BASE + 0x220 + (n) * 4)
#define POWER_ON						BIT(0)

#define PPU_WRAP(n)						(0x07051000 + (n) * 0x1000)
#define PPU_PWPR(n)						(PPU_WRAP(n) + 0x0000)
#define PPU_PMER(n)						(PPU_WRAP(n) + 0x0004)
#define PPU_PWSR(n)						(PPU_WRAP(n) + 0x0008)
#define STATE_ON						0x8
#define STATE_OFF						0x0

#define SUNXI_CLUS_OPS_BASE				0x08000000
#define SUNXI_INITARCH_REG(n)			(SUNXI_CLUS_OPS_BASE + 0x0020 + (n) * 4)
#define AARCH64							BIT(0)
#define SUNXI_CPUCFG_RVBAR_LO_REG(n)	(SUNXI_CLUS_OPS_BASE + 0x0040 + (n) * 8)
#define SUNXI_CPUCFG_RVBAR_HI_REG(n)	(SUNXI_CLUS_OPS_BASE + 0x0044 + (n) * 8)

typedef unsigned int uint32_t;
typedef unsigned long uintptr_t;

static inline void mmio_write_32(uintptr_t addr, uint32_t value)
{
	*(volatile uint32_t *)addr = value;
}

static inline uint32_t mmio_read_32(uintptr_t addr)
{
	return *(volatile uint32_t *)addr;
}

static inline void mmio_clrbits_32(uintptr_t addr, uint32_t clear)
{
	mmio_write_32(addr, mmio_read_32(addr) & ~clear);
}

static inline void mmio_setbits_32(uintptr_t addr, uint32_t set)
{
	mmio_write_32(addr, mmio_read_32(addr) | set);
}

#endif /* SUNXI_CPU_OPS_PRIVATE_H */
