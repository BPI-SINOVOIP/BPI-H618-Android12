/*
 * drivers/ir/ir.c
 *
 * Copyright (C) 2012-2016 AllWinnertech Ltd.
 * Author: Sunny <Sunny@allwinnertech.com>
 *
 */
#include "ir_i.h"
#include "include.h"
#include "irq_table.h"
#include <libfdt.h>

#if IR_USED
//#define IR_TEST


extern u32 dtb_base;
extern u32 volatile wakeup_source;

//ir configure information backup
static u32 ir_ctrl;
static u32 ir_rxcfg;
static u32 ir_rxinte;
static u32 ir_rxints;
static u32 ir_cfg;
static u32 ir_clk_src;
static u32 ir_clk_div;

static ir_key_t ir_key_depot = {0};

static u32 ir_valid_code = 0;
static u32 ir_valid_addr = 0;

//ir rx raw data buffer
static ir_raw_buffer_t raw_buffer;


static s32 ir_clk_cfg(void)
{
	u32 value;

	/* enable ir mode */
	value = 0x3 << 4;
	writel(value, IR_CTRL_REG);

	/* config ir configure register */
	value = 0;
#ifdef FPGA_PLATFORM
	value |= 3<<0;
	value |= ((16 & 0x3f) << 2);
	value |= ((5  & 0xff) << 8);
	value |= ((1  & 0xff) << 23);
#else
#if IR_24M_USED
	value |= IR_SAMPLE_128;
	value |= IR_RXFILT_VAL;
	value |= IR_RXIDLE_VAL;
	value |= IR_ACTIVE_T;
	value |= IR_ACTIVE_T_C;
#else
	value  = ((0x1 << 24) | (0x0 << 0));    //sample_freq = 32768Hz / 1 = 32768Hz (30.5us)
	value |= ((IR_RXFILT_VAL & 0x3f) << 2); //set filter threshold
	value |= ((IR_RXIDLE_VAL & 0xff) << 8); //set idle threshold
	value |= (ATHC_UNIT << 23);
	value |= (ATHC_THRE & 0x7f)<<16;
#endif
#endif
	writel(value, IR_CFG_REG);

	//invert input signal
	writel((0x1<<2), IR_RXCFG_REG);

	//clear all rx interrupt status
	writel(0xff, IR_RXINTS_REG);

	//set rx interrupt enable
	value = 0;
	value |=  (IR_RXINTS_RXDA);  /* enable fifo available interrupt */
	value |=  (IR_RXINTS_RXPE);  /* enable package end interrupt */
	value |=  (IR_RXINTS_RXOF);  /* enable fifo full interrupt */
	value |= ((IR_FIFO_TRIGER - 1) << 8);  /* rx fifo threshold = fifo-size / 2 */
	writel(value, IR_RXINTE_REG);

	//enable ir module
	value = readl(IR_CTRL_REG);
	value |= 0x3;
	writel(value, IR_CTRL_REG);

	return OK;
}

static int ir_sysconfig_cfg(void)
{
	ir_code_t ir_code;
	uint32_t count;
	int i;
	void *fdt = (void *)(dtb_base);
	int ir_node;
	int ret;
	uint32_t val;
	static u32 ir_has_parsed;

	if (!!ir_has_parsed)
		return 0;

	ir_node = fdt_path_offset(fdt, "cir_param");
	if (ir_node < 0) {
		ERR("no cir_param node in dts!");
		return ir_node;
	}

	/* parse count*/
	ret = fdt_getprop_u32(fdt, ir_node, "count", &count);
	if (ret < 0) {
		ERR("no count prop in ir_param node!");
		return ret;
	}

	char key_buf[32] = "ir_power_key_code";
	char add_buf[32] = "ir_addr_code";

	for (i = 0; i < count; i++) {
		sprintf(key_buf + 17, "%d", i);
		INF("%s\n", key_buf);
		sprintf(add_buf + 12, "%d", i);
		INF("%s\n", add_buf);

		/* parse ir_power_key_code*/
		ret = fdt_getprop_u32(fdt, ir_node, key_buf, &val);
		if (ret < 0) {
			ERR("no %s in ir_param node!", key_buf);
			return ret;
		}
		ir_code.key_code = (u32)val;

		/* parse ir_power_key_addr*/
		ret = fdt_getprop_u32(fdt, ir_node, add_buf, &val);
		if (ret < 0) {
			ERR("no %s in ir_param node!", add_buf);
			return ret;
		}
		ir_code.addr_code = (u32)val;
		ir_set_paras(&ir_code, i);
	}

	ir_has_parsed = 1;
	return 0;
}

static s32 ir_read_fifo_raw_data(struct ir_raw_buffer *raw_buf, u32 status)
{
	u32  i;
	u32  count;
	u8   data;
	u32  offset;

	/* read fifo byte count */
	count = (status >> 8) & 0x7f;
	/* read fifo */
	offset = raw_buf->count;
	if ((offset + count) >= IR_RAW_BUFFER_SIZE) {
		ERR("ir data full\n");
		return -EFULL;
	}
	for (i = 0; i < count; i++) {
		/* read fifo data */
		data = (u8)(readl(IR_RXDAT_REG) & 0xff);
		raw_buf->data[offset + i] = data;
	}
	raw_buf->count += i;

	return OK;
}

static u32 ir_decode_raw_data(u8 *buffer, u32 count)
{
	u32 len = 0;
	u32 val = 0, last = 1;
	u32 code = 0;
	u32 bitCnt = 0;
	u32 i = 0;

start_decode:

	if (i >= count) {
		return IR_ERROR_CODE;
	}

	/* skip unused value in the start */
	for (; i < count; i++) {
		val = buffer[i];
		if (val == 0xff)
			break;
	}

	/* find lead '1'*/
	for (len = 0; i < count; i++) {
		val = buffer[i];
		if (val & 0x80)
			len += (val & 0x7f);
		else {
			if (len > IR_L1_MIN)
				break;
			len = 0;
		}
	}
	if ((val & 0x80) || (len <= IR_L1_MIN)) {
		goto start_decode; /* Invalid Code */
	}

	/* find lead '0' */
	for (len = 0; i < count; i++) {
		val = buffer[i];
		if (val & 0x80) {
			if (len > IR_L0_MIN)
				break;
			len = 0;
		} else
			len += (val & 0x7f);
	}

	if ((!(val & 0x80)) || (len <= IR_L0_MIN)) {
		goto start_decode; /* Invalid Code */
	}

	/* go decoding */
	for (code = 0, bitCnt = 0, last = 1, len = 0; i < count; i++) {
		val = buffer[i];
		if (last) {
			if (val & 0x80)
				len += (val & 0x7f);
			else {
				if (len > IR_PMAX) {  /* error pulse */
					INF("Pulse %d\n", i);
					goto start_decode; /* Invalid Code */
				}
				last = 0;
				len = val & 0x7f;
			}
		} else {
			if (val & 0x80) {
				if (len > IR_DMAX) { /* error distant */
					goto start_decode; /* Invalid Code */
				} else {
					if (len > IR_DMID) /* data '1' */
						code |= 1<<bitCnt;
					bitCnt ++;
					if (bitCnt == 32)
						break; /* decode over */
				}
				last = 1;
				len = val & 0x7f;
			} else
				len += (val & 0x7f);
		}
	}
	return code;
}

static u32 ir_code_is_valid(u32 add_code, u32 code)
{
	u32 tmp1, tmp2;

#ifdef IR_CHECK_ADDR_CODE
	/* check address value */
	if((code & 0x0ffff) != (add_code & 0x0ffff))
		return FALSE; /* address error */
#endif
	tmp1 = code & 0x00ff0000;
	tmp2 = (code & 0xff000000)>>8;

	if ((tmp1^tmp2) == 0x00ff0000) {
		ir_valid_code = tmp1 >> 16;
		INF("detect valid ir code :%x\n", ir_valid_code);
		return TRUE;
	} else {
		ir_valid_code = 0;
		return FALSE;
	}
}

static s32 ir_verify_code(u32 status)
{
	u32 i;

	if (status & (0x1 << 4)) {
		/* rx fifo available interrupt */
		if (ir_read_fifo_raw_data(&raw_buffer, status) != OK) {
			//discard all received data
			WRN("read ir fifo raw data failed\n");
			raw_buffer.count = 0;
			goto error;
		}
	}
	if (status & (0x1 << 1)) {
		/* ir rx package end interrupt */
		if (ir_read_fifo_raw_data(&raw_buffer, status) != OK) {
			WRN("read ir fifo raw data failed\n");
			raw_buffer.count = 0;
			goto error;
		}

		/* decode raw data */
		ir_valid_addr = ir_decode_raw_data(raw_buffer.data, raw_buffer.count);
		raw_buffer.count = 0;

		/* check the code valid or not */
		for (i = 0; i < ir_key_depot.num; i++) {
			if (ir_code_is_valid(ir_key_depot.ir_code_depot[i].addr_code, ir_valid_addr)) {
				LOG("detect ir raw code :%x\n", ir_valid_addr);
				return TRUE;
			}
		}
		/* invalid code detect */
		goto error;
	}
	if (status & (0x1 << 0)) {
		/* ir rx fifo full interrupt */
		WRN("ir rx fifo full\n");
		raw_buffer.count = 0;
		goto error;
	}

error:
	writel(status, IR_RXINTS_REG);
	return FALSE;
}

u32 ir_is_power_key(void)
{
	int i;

	for (i = 0; i < ir_key_depot.num; i++) {
		if ((ir_valid_code == ir_key_depot.ir_code_depot[i].key_code) && \
		    ((ir_valid_addr & 0x0ffff) == ir_key_depot.ir_code_depot[i].addr_code)) {
			return TRUE;
		}
	}

	return FALSE;
}

static int ir_int_handler(void *parg)
{
	u32 intno = interrupt_get_current_intno();

	interrupt_clear_pending(INTC_R_IRRX_IRQ);

	u32 status = readl(IR_RXINTS_REG);

	if (!ir_verify_code(status)) {
		return FALSE;
	}
	if (ir_is_power_key()) {

		interrupt_set_mask(INTC_R_IRRX_IRQ, TRUE);

		wakeup_source = CPUS_IRQ_MAPTO_CPUX(intno);

		/* save ir code in RTC for kernel resume*/
		writel(ir_valid_addr, RTC_IR_CODE_STORE_REG);
		LOG("reset system now\n");
	} else {
		int i;
		writel(status & 0xff, IR_RXINTS_REG);
		for (i = 0; i < IR_FIFO_SIZE; i++)
			readl(IR_RXDAT_REG);
	}

	return TRUE;
}

s32 ir_init(void)
{
	INF("ir init\n");
	int i;
	int ret = ir_sysconfig_cfg();
	if (ret < 0){
		ERR("get dts cfg error");
		return OK;
	}

	//backup ir clock configure
	ir_clk_src = ccu_get_mclk_src(CCU_MOD_CLK_R_CIR);
	ir_clk_div = ccu_get_mclk_div(CCU_MOD_CLK_R_CIR);

	/* config R_CIR pin */
	pin_set_multi_sel(PIN_GRP_PL, 9, 3);             //PL4 config as R_CIR
	pin_set_pull(PIN_GRP_PL, 9, PIN_PULL_UP);        //R_CIR pull-up
	pin_set_drive(PIN_GRP_PL, 9, PIN_MULTI_DRIVE_0); //R_CIR drive level 0

	/* standby ir clock source=32k, div=1 or cource=24M, div=8 or
	 * IOSC/16/2/16=31250Hz=~32768Hz, 31250Hz can use 32768Hz paras, div = 1.
	 */
#if IR_24M_USED
	ccu_set_mclk_src(CCU_MOD_CLK_R_CIR, CCU_SYS_CLK_HOSC);
	ccu_set_mclk_onoff(CCU_MOD_CLK_R_CIR, CCU_CLK_ON);
	ccu_set_mclk_div(CCU_MOD_CLK_R_CIR, 8);
#else
	ccu_set_mclk_src(CCU_MOD_CLK_R_CIR, CCU_SYS_CLK_LOSC);
	ccu_set_mclk_onoff(CCU_MOD_CLK_R_CIR, CCU_CLK_ON);
	ccu_set_mclk_div(CCU_MOD_CLK_R_CIR, 1);
#endif
	//reset ir module
//	ccu_reset_module(CCU_MOD_CLK_APBS_R_CIR);
//	ccu_set_mclk_onoff(CCU_MOD_CLK_APBS_R_CIR, CCU_CLK_ON);

	//backup ir reg value
	ir_ctrl   = readl(IR_CTRL_REG);
	ir_rxcfg  = readl(IR_RXCFG_REG);
	ir_rxinte = readl(IR_RXINTE_REG);
	ir_rxints = readl(IR_RXINTS_REG);
	ir_cfg    = readl(IR_CFG_REG);

	//ir standby setup
	ir_clk_cfg();
	raw_buffer.count = 0;

	/* register ir interrupt handler */
	writel(0xef, IR_RXINTS_REG);
	install_isr(INTC_R_IRRX_IRQ, ir_int_handler, NULL);
	interrupt_clear_pending(INTC_R_IRRX_IRQ);
	interrupt_set_mask(INTC_R_IRRX_IRQ, FALSE);
	interrupt_enable(INTC_R_IRRX_IRQ);

	/* clean ir fifo buffer data */
	for (i = 0; i < IR_FIFO_SIZE; i++)
		readl(IR_RXDAT_REG);
	LOG("ir init success\n");
	return OK;
}

s32 ir_exit(void)
{
	//backup ir clock configure
	ccu_set_mclk_src(CCU_MOD_CLK_R_CIR, ir_clk_src);
	ccu_set_mclk_div(CCU_MOD_CLK_R_CIR, ir_clk_div);

	//reset ir module
//	ccu_reset_module(CCU_MOD_CLK_R_CIR);

	interrupt_disable(INTC_R_IRRX_IRQ);
	uninstall_isr(INTC_R_IRRX_IRQ, ir_int_handler);

	//restore ir registers
	writel(ir_ctrl   , IR_CTRL_REG);
	writel(ir_rxcfg  , IR_RXCFG_REG);
	writel(ir_rxinte , IR_RXINTE_REG);
	writel(ir_rxints , IR_RXINTS_REG);
	writel(ir_cfg    , IR_CFG_REG);

	return OK;
}

#ifdef KERNEL_USED
s32 ir_set_paras(struct message *pmessage)
{
	ir_key_depot.ir_code_depot[0].key_code = pmessage->paras[0];
	ir_key_depot.ir_code_depot[0].addr_code = pmessage->paras[1];
	ir_key_depot.num = 1;
	LOG("ir key:%x, addr:%x\n", ir_key_depot.ir_code_depot[0].key_code, \
	    ir_key_depot.ir_code_depot[0].addr_code);

	return OK;
}
#else
static int irkey_used = 0;

s32 ir_set_paras(ir_code_t *ir_code, int index)
{
	int i;

	/* check repeat configuration */
	for (i = 0; i < ir_key_depot.num; i++)
		if ((ir_key_depot.ir_code_depot[i].key_code == ir_code->key_code) && \
		    (ir_key_depot.ir_code_depot[i].addr_code == ir_code->addr_code))
			return OK;

	/* just cover the last ir config */
	if (ir_key_depot.num >= IR_NUM_KEY_SUP)
		ir_key_depot.num--;

	ir_key_depot.ir_code_depot[ir_key_depot.num].key_code = ir_code->key_code;
	ir_key_depot.ir_code_depot[ir_key_depot.num].addr_code = ir_code->addr_code;
	//LOG("ir num:%d, key:%x, add:%x\n", ir_key_depot.num, ir_code->key_code, ir_code->addr_code);
	ir_key_depot.num++;

	return OK;
}

u32 ir_is_used(void)
{
	return irkey_used;
}
#endif /* KERNEL_USED */

#ifdef IR_TEST
extern void pmu_reset_system(void);

s32 ir_test(void)
{
	u32 value = 0;

	ir_init();
	while (1) {
#ifdef FPGA_PLATFORM
		value = readl(IR_RXINTS_REG);
		if (value & 0x13) {
#else
		if (interrupt_query_pending(INTC_R_CIR_IRQ)) {
#endif
			LOG("ir detected\n");
			if (ir_is_power_key()) {
				LOG("cir ok\n");
				write_rtc_domain_reg(RTC_DM_REG3, 0x0f);
				pmu_reset_system();
			}
			writel(value, IR_RXINTS_REG);
		}
		time_mdelay(5);
	}

	ir_exit();
}
#endif
#endif /* IR_USED */
