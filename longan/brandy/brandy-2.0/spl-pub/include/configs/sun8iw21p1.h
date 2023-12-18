/*
 * (C) Copyright 2018
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * wangwei <wangwei@allwinnertech.com>
 */

#ifndef _SUN8IW21_H
#define _SUN8IW21_H

//#define FPGA_PLATFORM

#define BOOT_PUB_HEAD_VERSION  "3000"
#define CONFIG_ARCH_SUN8IW21
#define CONFIG_ARCH_SUN8IW21P1
#define CONFIG_SUNXI_NCAT_V2
#define CONFIG_DRAM_PARA_V1

/* sram layout*/
#define CONFIG_SYS_SRAM_BASE             (0x20000)
// #define CONFIG_SYS_SRAM_SIZE             (0x8000)
// #define CONFIG_SYS_SRAMA2_BASE           (0x400000)
// #define CONFIG_SYS_SRAMA2_SIZE           (0x20000)
#define CONFIG_SYS_SRAMC_BASE            (0x20000)
#define CONFIG_SYS_SRAMC_SIZE            (0x21000)

/* dram layout*/
#define SDRAM_OFFSET(x)                   (0x40000000+(x))
#define CONFIG_SYS_DRAM_BASE              SDRAM_OFFSET(0)
#define DRAM_PARA_STORE_ADDR              SDRAM_OFFSET(0x00800000) /*fel*/      /*same as base.h*/
#define CONFIG_HEAP_BASE                  SDRAM_OFFSET(0x00800000) /*secure */ /*same as base.h*/
#define CONFIG_HEAP_SIZE                  (16 * 1024 * 1024)  /*same as base.h*/

#define CONFIG_BOOTPKG_BASE               SDRAM_OFFSET(0x02e00000) /*same as base.h*/

#define SUNXI_DRAM_PARA_MAX               32

/*same as base.h*/
#define SUNXI_LOGO_COMPRESSED_LOGO_SIZE_ADDR            SDRAM_OFFSET(0x03000000)
#define SUNXI_LOGO_COMPRESSED_LOGO_BUFF                 SDRAM_OFFSET(0x03000010)
#define SUNXI_SHUTDOWN_CHARGE_COMPRESSED_LOGO_SIZE_ADDR SDRAM_OFFSET(0x03100000)
#define SUNXI_SHUTDOWN_CHARGE_COMPRESSED_LOGO_BUFF  	SDRAM_OFFSET(0x03100010)
#define SUNXI_ANDROID_CHARGE_COMPRESSED_LOGO_SIZE_ADDR  SDRAM_OFFSET(0x03200000)
#define SUNXI_ANDROID_CHARGE_COMPRESSED_LOGO_BUFF   	SDRAM_OFFSET(0x03200010)

/* boot run addr */
#define FEL_BASE                          0x20
#define SECURE_FEL_BASE                  (0x64)
#define CONFIG_BOOT0_RUN_ADDR            (0x20000)  /* sram a */
#define CONFIG_NBOOT_STACK               (CONFIG_SYS_SRAMC_BASE+CONFIG_SYS_SRAMC_SIZE)
#define CONFIG_TOC0_RUN_ADDR             (0x20480)  /* sram a */
#define CONFIG_HASH_TABLE_STACK_GAP      (4)
#define CONFIG_HASH_INFO_TABLE_SIZE      (512 - CONFIG_HASH_TABLE_STACK_GAP)/*to pass hash info to optee*/
#define CONFIG_HASH_INFO_TABLE_BASE      (CONFIG_SYS_SRAMC_BASE + CONFIG_SYS_SRAMC_SIZE - CONFIG_HASH_INFO_TABLE_SIZE)
#define CONFIG_SBOOT_STACK               (CONFIG_HASH_INFO_TABLE_BASE - CONFIG_HASH_TABLE_STACK_GAP)
#define CONFIG_BOOT0_RET_ADDR            (CONFIG_SYS_SRAM_BASE)
#define CONFIG_TOC0_HEAD_BASE            (CONFIG_SYS_SRAM_BASE)
#define CONFIG_TOC0_CFG_ADDR             (CONFIG_SYS_SRAM_BASE + 0x80)

/* handle standby */
#define RTC_STANDBY_FLAG_REG           (SUNXI_RTC_BASE + 0x1f8)
#define RTC_STANDBY_SOFT_ENTRY_REG     (SUNXI_RTC_BASE + 0x1fc)

/* rtc check power off*/
#define FORCE_DETECTER_OUTPUT		  (1 << 7)
#define VCCIO_THRESHOLD_VOLTAGE_2_5	  (0 << 4)
#define VCCIO_THRESHOLD_VOLTAGE_2_6	  (1 << 4)
#define VCCIO_THRESHOLD_VOLTAGE_2_7	  (2 << 4)
#define VCCIO_THRESHOLD_VOLTAGE_2_8	  (3 << 4)
#define VCCIO_THRESHOLD_VOLTAGE_2_9	  (4 << 4)
#define VCCIO_THRESHOLD_VOLTAGE_3_0	  (5 << 4)
#define VCCIO_DET_BYPASS_EN		  (1 << 0)

/* for dram crc check */
#define CRC_VALUE_BEFORE               3
#define CRC_EN                         0
#define CRC_START                      6
#define CRC_LEN                        7


#define CONFIG_SYS_INIT_RAM_SIZE         (CONFIG_SYS_SRAM_SIZE)

/* FES */
#define CONFIG_FES1_RUN_ADDR             (0x28000)
#define CONFIG_FES1_RET_ADDR             (CONFIG_SYS_SRAMC_BASE + 0x7210)

/*CPU vol for boot*/
#define CONFIG_SUNXI_CORE_VOL           900

/* watchdog */
#define WDOG_TIMEOUT                    16

#endif
