/*
 * drivers/uart/uart.c
 *
 * Copyright (C) 2012-2016 AllWinnertech Ltd.
 * Author: Sunny <Sunny@allwinnertech.com>
 *
 */
#include "uart_i.h"
#include <libfdt.h>

volatile u32 uart_pin_not_used = 1;
volatile u32 uart_lock = 1;
volatile static u32 uart_rate;

extern u32 dtb_base;

#define IS_TX_FIFO_EMPTY  (readl(UART_REG_USR) & (0x1 << 2))

#ifdef CFG_FDT_INIT_ARISC_USED
static s32 uart_init_from_dts(void)
{
	void *fdt;
	int arisc_node, arisc_uart_node;
	int len, i, ret;
	u32 pin_grp = 0;
	u32 pin_num = 0;
	u32 count = 0;
	uint32_t pin_func;
	uint32_t array[8];
	const char *pin_char;
	char pin_num_string[4];

	fdt = (void *)(dtb_base);

	arisc_node = fdt_path_offset(fdt, "arisc-config");
	if (arisc_node < 0)
		return arisc_node;

	arisc_uart_node = fdt_subnode_offset(fdt, arisc_node, "s_uart_config");
	if (arisc_uart_node < 0)
		return arisc_uart_node;

	ret = fdt_get_status(fdt, arisc_uart_node);
	if (!ret)
		return -1;

	count = fdt_stringlist_count(fdt, arisc_uart_node, "pins");

	ret = fdt_get_u32_array(fdt, arisc_uart_node, "function", array, count);
	if (ret < 0)
		return ret;

	for (i = 0; i < count; i++) {
		pin_char = fdt_stringlist_get(fdt, arisc_uart_node, "pins", i, &len);
		if (strncmp(pin_char, "PL", strlen("PL")) == 0) {
			pin_grp = PIN_GRP_PL;
			strncpy(pin_num_string, pin_char + strlen("PL"), 2);
			pin_num = dstr2int(pin_num_string, 2);
		} else if (strncmp(pin_char, "PM", strlen("PM")) == 0) {
			pin_grp = PIN_GRP_PM;
			strncpy(pin_num_string, pin_char + strlen("PM"), 2);
			pin_num = dstr2int(pin_num_string, 2);
		}

		pin_func = array[i];
		pin_set_multi_sel(pin_grp, pin_num, pin_func);
	}

	uart_pin_not_used = 0;
	return 0;
}
#endif

s32 uart_clkchangecb(u32 command, u32 freq)
{
	if (uart_pin_not_used)
		return -EACCES;

	switch (command) {
	case CCU_CLK_CLKCHG_REQ:
		{
			/* clock will be change */
			INF("uart source clock change request\n");
			/* wait uart transmit fifo empty */
			while (!IS_TX_FIFO_EMPTY)
				continue;
			/* lock uart */
			uart_lock = 1;
			return OK;
		}
	case CCU_CLK_CLKCHG_DONE:
		{
			/* reconfig uart current baudrate */
			uart_set_baudrate(uart_rate);
			uart_lock = 0;
			INF("uart buadrate change done\n");
			return OK;
		}
	}

	return -ESRCH;
}

/*
 * uart_init() - initialize uart.
 *
 * @return: OK if initialize uart succeeded, others if failed.
 * @note: initialize uart.
 */
s32 uart_init(void)
{
	u32 div;
	u32 lcr;
	u32 apb0_clk;

	uart_rate = UART_BAUDRATE;
#ifndef CFG_FDT_INIT_ARISC_USED
	pin_set_multi_sel(PIN_GRP_PL, 2, 2);
	pin_set_multi_sel(PIN_GRP_PL, 3, 2);
#else
	if (uart_init_from_dts() < 0)
		uart_pin_not_used = 1;
#endif

	if (uart_pin_not_used)
		return -EACCES;

	/* set reset as assert state. */
	ccu_set_mclk_reset(CCU_MOD_CLK_R_UART, CCU_CLK_NRESET),

	/* set uart clock, open apb0 uart gating. */
	ccu_set_mclk_onoff(CCU_MOD_CLK_R_UART, CCU_CLK_ON);

	/* the baud rate divisor */
#ifndef CFG_FPGA_PLATFORM
	apb0_clk = ccu_get_sclk_freq(CCU_SYS_CLK_APBS2);
#else
	apb0_clk = 24000000;
#endif
	div = (apb0_clk + (uart_rate << 3)) / (uart_rate << 4);

	/* initialize uart controller */
	lcr = readl(UART_REG_LCR);
	writel(lcr | 0x80, UART_REG_LCR);  /* select the Divsor Latch Low Register and Divsor Latch High Register */
	writel(div & 0xff, UART_REG_DLL);
	writel((div >> 8) & 0xff, UART_REG_DLH);
#ifdef CFG_SHELL_USED
	writel(lcr & (~0x80), UART_REG_LCR);
#endif
	writel(0, UART_REG_HALT);
	writel(3, UART_REG_LCR);           /* set mode, 8bit charset */
	writel(7, UART_REG_FCR);           /* enable and reset transmit/receive fifo */
#ifdef CFG_SHELL_USED
	writel(1, UART_REG_IER);       /* enable receiver interrupt */
#endif
	/* ensure uart is unlock */
	uart_lock = 0;

	/* uart initialize succeeded */
	return OK;
}

/*
 * uart_putc() - exit uart.
 *
 * @return: OK if exit uart succeeded, others if failed.
 * @note: exit uart.
 */
s32 uart_exit(void)
{
	uart_lock = 1;
	uart_pin_not_used = 1;

	pin_set_multi_sel(PIN_GRP_PL, 2, 7);
	pin_set_multi_sel(PIN_GRP_PL, 3, 7);

	/* set uart clock, open apb0 uart gating. */
	ccu_set_mclk_onoff(CCU_MOD_CLK_R_UART, CCU_CLK_OFF);

	/* set reset as assert state. */
	ccu_set_mclk_reset(CCU_MOD_CLK_R_UART, CCU_CLK_RESET);

	return OK;
}

/*
 * uart_putc() - put out a charset.
 *
 * @ch: the charset which we want to put out.
 * @return: OK if put out charset succeeded, others if failed.
 * @note: put out a charset.
 */
s32 uart_putc(char ch)
{
	if (uart_lock || uart_pin_not_used)
		return -EACCES;

	while (!(readl(UART_REG_USR) & 2)) /* fifo is full, check again */
		;

	/* write out charset to transmit fifo */
	writel(ch, UART_REG_THR);

	return OK;
}

/*
 * uart_getc() - get a charset.
 *
 * @return: the charset we read from uart.
 * @note: get a charset.
 */
u32 uart_get(char *buf)
{
	u32 count = 0;

	if (uart_lock || uart_pin_not_used)
		return -EACCES;

	while (readl(UART_REG_RFL)) {
		*buf++ = (char)(readl(UART_REG_RBR) & 0xff);
		count++;
	}

	return count;
}

/*
 * uart_puts() - put out a string.
 *
 * @string: the string which we want to put out.
 * @return: OK if put out string succeeded, others if failed.
 * @note: put out a string.
 */
s32 uart_puts(const char *string)
{
	if (uart_lock || uart_pin_not_used)
		return -EACCES;

	ASSERT(string != NULL);

	while (*string != '\0') {
		if (*string == '\n') /* if current character is '\n', insert output with '\r' */
			uart_putc('\r');
		uart_putc(*string++);
	}

	return OK;
}

s32 uart_set_baudrate(u32 rate)
{
	u32 freq;
	u32 div;
	u32 lcr;
	u32 halt;

	if (uart_pin_not_used)
		return -EACCES;

	/* update current uart baudrate */
	LOG("uart buadrate change from [%d] to [%d]\n", uart_rate, rate);
	uart_rate = rate;

	/* wait uart transmit fifo empty */
	while (readl(UART_REG_TFL))
		continue;

	/* reconfig uart baudrate */
#ifdef CFG_FPGA_PLATFORM
	freq = 24000000;
#else
	freq = ccu_get_sclk_freq(CCU_SYS_CLK_APBS2);
#endif
	div  = (freq + (uart_rate << 3)) / (uart_rate << 4);
	lcr  = readl(UART_REG_LCR);

	if (div == 0) /* avoid config invalid value */
		div = 1;

	/* enable change when busy */
	halt = readl(UART_REG_HALT) | 0x2;
	writel(halt, UART_REG_HALT);

	/* select the divsor latch low register and divsor latch high register */
	writel(lcr | 0x80, UART_REG_LCR);

	/* set divsor of uart */
	writel(div & 0xff, UART_REG_DLL);
	writel((div >> 8) & 0xff, UART_REG_DLH);

	/* unselect the divsor latch low register and divsor latch high register */
	writel(lcr & (~0x80), UART_REG_LCR);

	/* update baudrate */
	halt = readl(UART_REG_HALT) | 0x4;
	writel(halt, UART_REG_HALT);

	/* waiting update */
	while (readl(UART_REG_HALT) & 0x4)
		continue;

	/* disable change when busy */
	halt = readl(UART_REG_HALT) | 0x4;
	writel(halt & (~0x2), UART_REG_HALT);

	return OK;
}

u32 uart_get_baudrate(void)
{
	return uart_rate;
}
