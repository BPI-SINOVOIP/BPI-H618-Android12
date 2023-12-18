/*
 * (C) Copyright 2018
 * wangwei <wangwei@allwinnertech.com>
 */

#include <common.h>
#include <private_boot0.h>
#include <private_uboot.h>
#include <private_toc.h>
#include <arch/clock.h>
#include <arch/uart.h>
#include <arch/dram.h>
#include <arch/rtc.h>

static void  note_dram_log(int dram_init_flag);


int main(void)
{
	int dram_size=0;
	int status;
	sunxi_serial_init(fes1_head.prvt_head.uart_port, (void *)fes1_head.prvt_head.uart_ctrl, 2);
	printf("fes begin commit:%s\n", fes1_head.hash);
	status = sunxi_board_init();
	if(status)
		return 0;

	printf("beign to init dram\n");
#ifdef FPGA_PLATFORM
	dram_size = mctl_init((void *)fes1_head.prvt_head.dram_para);
#else
	dram_size = init_DRAM(0, (void *)fes1_head.prvt_head.dram_para);
#endif
	if (rtc_probe_fel_flag()) {
		rtc_clear_fel_flag();
	}
	if (dram_size) {
		note_dram_log(1);
		printf("init dram ok\n");
	} else {
		note_dram_log(0);
		printf("init dram fail\n");
#ifdef CFG_ARCH_RISCV
		asm volatile("j .");
#else
		asm volatile("b .");
#endif
	}

	mdelay(10);

	return dram_size;
}

static void  note_dram_log(int dram_init_flag)
{
	fes_aide_info_t *fes_aide = (fes_aide_info_t *)&fes1_head.fes_union_addr.fes1_res_addr;

	memset(fes_aide, 0, sizeof(fes_aide_info_t));
	fes_aide->dram_init_flag    = SYS_PARA_LOG;
	fes_aide->dram_update_flag  = dram_init_flag;

	memcpy(fes_aide->dram_paras, fes1_head.prvt_head.dram_para, SUNXI_DRAM_PARA_MAX * 4);
	memcpy((void *)DRAM_PARA_STORE_ADDR, fes1_head.prvt_head.dram_para, SUNXI_DRAM_PARA_MAX * 4);
}
