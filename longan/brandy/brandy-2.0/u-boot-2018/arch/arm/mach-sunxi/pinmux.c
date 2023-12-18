// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2007-2011
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Tom Cubie <tangliang@allwinnertech.com>
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/gpio.h>

void sunxi_gpio_set_cfgbank(struct sunxi_gpio *pio, int bank_offset, u32 val)
{
	u32 index = GPIO_CFG_INDEX(bank_offset);
	u32 offset = GPIO_CFG_OFFSET(bank_offset);

	clrsetbits_le32(&pio->cfg[0] + index, 0xf << offset, val << offset);
}

void sunxi_gpio_set_cfgpin(u32 pin, u32 val)
{
	u32 bank = GPIO_BANK(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	sunxi_gpio_set_cfgbank(pio, pin, val);
}

int sunxi_gpio_get_cfgbank(struct sunxi_gpio *pio, int bank_offset)
{
	u32 index = GPIO_CFG_INDEX(bank_offset);
	u32 offset = GPIO_CFG_OFFSET(bank_offset);
	u32 cfg;

	cfg = readl(&pio->cfg[0] + index);
	cfg >>= offset;

	return cfg & 0xf;
}

int sunxi_gpio_get_cfgpin(u32 pin)
{
	u32 bank = GPIO_BANK(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	return sunxi_gpio_get_cfgbank(pio, pin);
}

int sunxi_gpio_set_drv(u32 pin, u32 val)
{
	u32 bank = GPIO_BANK(pin);
	u32 index = GPIO_DRV_INDEX(pin);
	u32 offset = GPIO_DRV_OFFSET(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	clrsetbits_le32(&pio->drv[0] + index, 0x3 << offset, val << offset);

	return 0;
}

int sunxi_gpio_set_pull(u32 pin, u32 val)
{
	u32 bank = GPIO_BANK(pin);
	u32 index = GPIO_PULL_INDEX(pin);
	u32 offset = GPIO_PULL_OFFSET(pin);
	struct sunxi_gpio *pio = BANK_TO_GPIO(bank);

	clrsetbits_le32(&pio->pull[0] + index, 0x3 << offset, val << offset);

	return 0;
}

#if defined(PIOC_REG_POW_MS_CTL) && \
	defined (PIOC_REG_POW_MOD_SEL) && \
	defined (PIOC_REG_POW_VAL)

__weak int get_group_bit_offset(enum pin_e port_group)
{
	return -1;
}

void io_pow_mode_disable(enum pin_e port_group)
{
	uint32_t reg;
	uint8_t group_bit_offset = get_group_bit_offset(port_group);
	if (group_bit_offset < 0)
		return;
	//disable auto mode
	reg = readl(PIOC_REG_POW_MS_CTL);
	reg &= ~(1 << group_bit_offset);
	reg |= (PIOC_CTL_Px_DISABLE << group_bit_offset);
	writel(reg, PIOC_REG_POW_MS_CTL);
}

enum io_pow_mode_e io_get_volt_val(enum pin_e port_group)
{
	uint32_t reg;
	uint8_t group_bit_offset = get_group_bit_offset(port_group);
	if (group_bit_offset < 0)
		return IO_MODE_DEFAULT;

	reg = readl(PIOC_REG_POW_VAL);
	return ((reg & (1 << group_bit_offset)) != PIOC_VAL_Px_3_3V_VOL) ?
		       IO_MODE_1_8_V :
		       IO_MODE_3_3_V;
}

void io_set_pow_mode(enum pin_e port_group, enum io_pow_mode_e volt_mode)
{
	uint32_t reg;
	uint8_t group_bit_offset = get_group_bit_offset(port_group);
	if (group_bit_offset < 0)
		return;
	if (volt_mode == IO_MODE_DEFAULT) {
		//default vaule in spec
		reg = readl(PIOC_REG_POW_MS_CTL);
		reg &= ~(1 << group_bit_offset);
		reg |= (PIOC_CTL_Px_DEFUALT << group_bit_offset);
		writel(reg, PIOC_REG_POW_MS_CTL);

		reg = readl(PIOC_REG_POW_MOD_SEL);
		reg &= ~(1 << group_bit_offset);
		reg |= (PIOC_SEL_Px_DEFAULT << group_bit_offset);
		writel(reg, PIOC_REG_POW_MOD_SEL);
	} else if (volt_mode == IO_MODE_AUTO) {
		//one specific combination is auto
		reg = readl(PIOC_REG_POW_MS_CTL);
		reg &= ~(1 << group_bit_offset);
		reg |= (PIOC_CTL_Px_ENABLE << group_bit_offset);
		writel(reg, PIOC_REG_POW_MS_CTL);

		reg = readl(PIOC_REG_POW_MOD_SEL);
		reg &= ~(1 << group_bit_offset);
		reg |= (PIOC_SEL_Px_3_3V_VOL << group_bit_offset);
		writel(reg, PIOC_REG_POW_MOD_SEL);
	} else {
		if (volt_mode == IO_MODE_1_8_V) {
			reg = readl(PIOC_REG_POW_MOD_SEL);
			reg &= ~(1 << group_bit_offset);
			reg |= (PIOC_SEL_Px_1_8V_VOL << group_bit_offset);
			writel(reg, PIOC_REG_POW_MOD_SEL);
		} else {
			reg = readl(PIOC_REG_POW_MOD_SEL);
			reg &= ~(1 << group_bit_offset);
			reg |= (PIOC_SEL_Px_3_3V_VOL << group_bit_offset);
			writel(reg, PIOC_REG_POW_MOD_SEL);
		}
	}
}
void sunxi_io_set_pow_mode_on_actual_val(enum pin_e port_group)
{
	io_pow_mode_disable(port_group);

	io_set_pow_mode(port_group, io_get_volt_val(port_group));
}

void sunxi_io_set_pow_mode_to_default(enum pin_e port_group)
{
	io_set_pow_mode(port_group, IO_MODE_DEFAULT);
}

#endif
