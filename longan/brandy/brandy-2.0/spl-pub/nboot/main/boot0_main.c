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
#include <arch/gpio.h>
#include <board_helper.h>
#include <config.h>

static void update_uboot_info(phys_addr_t uboot_base, phys_addr_t optee_base,
		phys_addr_t monitor_base, phys_addr_t rtos_base, u32 dram_size,
		u16 pmu_type, u16 uart_input, u16 key_input);
static int boot0_clear_env(void);
__maybe_unused int load_kernel_from_spinor(u32 *);
__maybe_unused void startup_kernel(u32, u32);

static void print_commit_log(void)
{
	printf("HELLO! BOOT0 is starting!\n");
	printf("BOOT0 commit : %s\n", BT0_head.hash);
	sunxi_set_printf_debug_mode(BT0_head.prvt_head.debug_mode, 0);
}

void main(void)
{
	int dram_size;
	int status;
	phys_addr_t  uboot_base = 0, optee_base = 0, monitor_base = 0, \
				rtos_base = 0, opensbi_base = 0, cpus_rtos_base = 0;
	u16 pmu_type = 0, key_input = 0; /* TODO: set real value */

	sunxi_board_init_early();
	sunxi_serial_init(BT0_head.prvt_head.uart_port, (void *)BT0_head.prvt_head.uart_ctrl, 6);
	print_commit_log();

	status = sunxi_board_init();
	if (status)
		goto _BOOT_ERROR;

	if (rtc_probe_fel_flag()) {
		rtc_clear_fel_flag();
		goto _BOOT_ERROR;
#ifdef CFG_SUNXI_PHY_KEY
#ifdef CFG_LRADC_KEY
	} else if (check_update_key(&key_input)) {
		goto _BOOT_ERROR;
#endif
#endif
	}

	if (BT0_head.prvt_head.enable_jtag) {
		printf("enable_jtag\n");
		boot_set_gpio((normal_gpio_cfg *)BT0_head.prvt_head.jtag_gpio, 5, 1);
	}

	char uart_input_value = get_uart_input(); /* Prevent DRAM jamming */
	if (uart_input_value == '2') {
		printf("detected_f user input 2\n");
		goto _BOOT_ERROR;
	}

#ifdef FPGA_PLATFORM
	dram_size = mctl_init((void *)BT0_head.prvt_head.dram_para);
#else
	dram_size = init_DRAM(0, (void *)BT0_head.prvt_head.dram_para);
#endif
	if (!dram_size) {
		printf("init dram fail\n");
		goto _BOOT_ERROR;
	} else {
		if (BT0_head.dram_size > 0)
			dram_size = BT0_head.dram_size;
		printf("dram size =%d\n", dram_size);
	}
#ifdef CFG_SUNXI_STANDBY_WORKAROUND
	handler_super_standby();
#endif

	uart_input_value = get_uart_input();
	if (uart_input_value == '2') {
		sunxi_set_printf_debug_mode(3, 0);
		printf("detected_r user input 2\n");
		goto _BOOT_ERROR;
	} else if (uart_input_value == 'd') {
		sunxi_set_printf_debug_mode(8, 1);
		printf("detected user input d\n");
	} else if (uart_input_value == 'q') {
		printf("detected user input q\n");
		sunxi_set_printf_debug_mode(0, 1);
	}

	mmu_enable(dram_size);
	malloc_init(CONFIG_HEAP_BASE, CONFIG_HEAP_SIZE);
	status = sunxi_board_late_init();
	if (status)
		goto _BOOT_ERROR;

	status = load_package();
	if (status == 0)
		load_image(&uboot_base, &optee_base, &monitor_base, &rtos_base, &opensbi_base, &cpus_rtos_base);
	else
		goto _BOOT_ERROR;
#ifdef CFG_RISCV_E907
	extern void boot_e907(phys_addr_t base, unsigned long fdt_addr);
	if (cpus_rtos_base != 0)
		boot_e907(cpus_rtos_base, (unsigned long)working_fdt);
#endif
	update_uboot_info(uboot_base, optee_base, monitor_base, rtos_base, dram_size,
			pmu_type, uart_input_value, key_input);
	mmu_disable( );

#if CFG_BOOT0_LOAD_KERNEL
	exist_uboot_jmp_cardproduct(uboot_base);
	void load_and_run_kernel(u32 optee_base, u32 opensbi_base);
	load_and_run_kernel(optee_base, opensbi_base);
	//already jump to kernel, should never return, if so, go FEL
	goto _BOOT_ERROR;
#endif

	printf("Jump to second Boot.\n");
	if (opensbi_base) {
			boot0_jmp_opensbi(opensbi_base, uboot_base);
	} else if (monitor_base) {
		struct spare_monitor_head *monitor_head =
			(struct spare_monitor_head *)((phys_addr_t)monitor_base);
		monitor_head->secureos_base = optee_base;
		monitor_head->nboot_base = uboot_base;
		boot0_jmp_monitor(monitor_base);
	} else if (optee_base)
		boot0_jmp_optee(optee_base, uboot_base);
	else if (rtos_base) {
		printf("jump to rtos\n");
		boot0_jmp(rtos_base);
	}
	else
		boot0_jmp(uboot_base);

	while(1);
_BOOT_ERROR:
	boot0_clear_env();
	boot0_jmp(FEL_BASE);

}

static void update_uboot_info(phys_addr_t uboot_base, phys_addr_t optee_base,
		phys_addr_t monitor_base, phys_addr_t rtos_base, u32 dram_size,
		u16 pmu_type, u16 uart_input, u16 key_input)
{
	if (rtos_base)
		return;

	uboot_head_t  *header = (uboot_head_t *) uboot_base;
	struct sbrom_toc1_head_info *toc1_head = (struct sbrom_toc1_head_info *)CONFIG_BOOTPKG_BASE;

	header->boot_data.boot_package_size = toc1_head->valid_len;
	header->boot_data.dram_scan_size = dram_size;
	memcpy((void *)header->boot_data.dram_para, &BT0_head.prvt_head.dram_para, 32 * sizeof(int));

#ifdef CFG_SUNXI_FDT
	sunxi_update_fdt_para_for_kernel();
#endif
	if (monitor_base)
		header->boot_data.monitor_exist = 1;
	if (optee_base)
		header->boot_data.secureos_exist = 1;
#ifndef CONFIG_RISCV
	header->boot_data.func_mask |= get_uboot_func_mask(UBOOT_FUNC_MASK_ALL);
#endif
	update_flash_para(uboot_base);

	header->boot_data.uart_port = BT0_head.prvt_head.uart_port;
	memcpy((void *)header->boot_data.uart_gpio, BT0_head.prvt_head.uart_ctrl, 2*sizeof(normal_gpio_cfg));
	header->boot_data.pmu_type = pmu_type;
	header->boot_data.uart_input = uart_input;
	header->boot_data.key_input = key_input;
	header->boot_data.debug_mode = sunxi_get_debug_mode_for_uboot();
}

static int boot0_clear_env(void)
{
	sunxi_board_exit();
	sunxi_board_clock_reset();
	mmu_disable();
	mdelay(10);

	return 0;
}

#ifdef CFG_KERNEL_BOOTIMAGE
/*
 * CFG_BOOT0_LOAD_KERNEL=y
 * CFG_KERNEL_BOOTIMAGE=y
 * CFG_KERNEL_CHECKSUM=n #y will check kernel checksum in bimage, but slower
 * CFG_SPINOR_KERNEL_OFFSET=0x20 #first partition, 0x20
 * CFG_SPINOR_LOGICAL_OFFSET=2016
 * CFG_KERNEL_LOAD_ADDR=0x41008000
 * CFG_SUNXI_FDT_ADDR=0x41800000
 */

#ifdef CFG_SUNXI_SPINOR
#if !defined(CFG_SPINOR_LOGICAL_OFFSET) ||                                     \
	!defined(CFG_SPINOR_KERNEL_OFFSET) ||                                  \
	!defined(CFG_KERNEL_LOAD_ADDR) || !defined(CFG_SUNXI_FDT_ADDR)
#error CFG_KERNEL_LOAD_ADDR CFG_SPINOR_LOGICAL_OFFSET CFG_SPINOR_KERNEL_OFFSET CFG_SUNXI_FDT_ADDR \
required for boot kernel function !

#endif
#define LOGICAL_OFFSET    CFG_SPINOR_LOGICAL_OFFSET
#define KERNEL_OFFSET     CFG_SPINOR_KERNEL_OFFSET
#endif /* CFG_SUNXI_SPINOR */

#ifdef CFG_SUNXI_SDMMC
#if !defined(CFG_MMC_KERNEL_OFFSET) ||                                     \
	!defined(CFG_MMC_LOGICAL_OFFSET) ||                                  \
	!defined(CFG_KERNEL_LOAD_ADDR) || !defined(CFG_SUNXI_FDT_ADDR)
#error CFG_KERNEL_LOAD_ADDR CFG_MMC_KERNEL_OFFSET CFG_MMC_LOGICAL_OFFSET CFG_SUNXI_FDT_ADDR \
required for boot kernel function !
#endif

#define LOGICAL_OFFSET    CFG_MMC_LOGICAL_OFFSET
#define KERNEL_OFFSET     CFG_MMC_KERNEL_OFFSET

int sunxi_mmc_exit(int sdc_no, const normal_gpio_cfg *gpio_info, int offset);
int get_card_num(void);
#endif /* CFG_SUNXI_SDMMC */

#if !defined(CFG_SUNXI_SPINOR) &&                                     \
	!defined(CFG_SUNXI_SDMMC)
#error Only supports mmc and nor
#endif

/* Failed to start or read the flag
in the flash to enter the system backup*/
int load_kernel_from_flash(u32 *kernel_addr,
			   int (*flash_read)(uint, uint, void *))
{
	int ret = 0;
#ifdef CFG_KERNEL_BOOTIMAGE
	u32 kernel_start = KERNEL_OFFSET + LOGICAL_OFFSET;
#ifdef CFG_SPINOR_KERNEL_BACKUP_OFFSET
	u32 kernel_backup =
			CFG_SPINOR_KERNEL_BACKUP_OFFSET + LOGICAL_OFFSET;
	if (!get_switch_kernel_flag()) {
		printf("use kernel backup\n");
		ret = load_bimage(kernel_backup, CFG_KERNEL_LOAD_ADDR,
				  flash_read, kernel_addr);
		if (ret < 0) {
			printf("bootimage backup0 erro\n");
		}
	} else {
#endif /*CFG_SPINOR_KERNEL_BACKUP_OFFSET*/
		ret = load_bimage(kernel_start, CFG_KERNEL_LOAD_ADDR,
				  flash_read, kernel_addr);
		if (ret < 0) {
			printf("load bootimage erro\n");
//CFG_SPINOR_KERNEL_BACKUP_OFFSET=0x37A0
#ifdef CFG_SPINOR_KERNEL_BACKUP_OFFSET
			ret = load_bimage(kernel_backup, CFG_KERNEL_LOAD_ADDR,
					  flash_read, kernel_addr);
			if (ret < 0) {
				printf("bootimage backup0 erro\n");
			}

		}
#endif /*CFG_SPINOR_KERNEL_BACKUP_OFFSET*/
	}
#endif

//CFG_KERNEL_UIMAGE=y
#ifdef CFG_KERNEL_UIMAGE
#define UIMAGE_SIZE_KB (2 * 1024)
	ret = load_uimage(KERNEL_OFFSET + LOGICAL_OFFSET,
			  CFG_KERNEL_LOAD_ADDR, UIMAGE_SIZE_KB, spinor_read);
	if (ret < 0) {
		printf("load uimage erro\n");
	}
#endif

	return ret;
}

void load_and_run_kernel(u32 optee_base, u32 opensbi_base)
{
	int load_flash_success = -1;
	u32 kernel_addr;
	u32 kernel_runner_base;
	int (*flash_read)(uint, uint, void *);
#ifdef CFG_SUNXI_SDMMC
	flash_read = mmc_bread_ext;
#elif CFG_SUNXI_SPINOR
	flash_read = spinor_read;
#endif

	load_flash_success = load_kernel_from_flash(&kernel_addr, flash_read);
	if (load_flash_success)
		return;

#ifdef CFG_ARCH_RISCV
	kernel_runner_base = opensbi_base;
#else
	kernel_runner_base = optee_base;
#endif

#ifdef CFG_SUNXI_SDMMC
	sunxi_mmc_exit(get_card_num(), BT0_head.prvt_head.storage_gpio, 16);
#endif

	if (kernel_runner_base) {
		optee_jump_kernel(kernel_runner_base, CFG_SUNXI_FDT_ADDR,
				  kernel_addr);
	} else {
		boot0_jump_kernel(CFG_SUNXI_FDT_ADDR, kernel_addr);
	}
}

#endif /* CFG_KERNEL_BOOTIMAGE */
