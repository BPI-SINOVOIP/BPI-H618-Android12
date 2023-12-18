/*
*********************************************************************************************************
*                                                AR100 SYSTEM
*                                     AR100 Software System Develop Kits
*                                                 io module
*
*                                    (c) Copyright 2012-2016, Sunny China
*                                             All Rights Reserved
*
* File    : io.h
* By      : Sunny
* Version : v1.0
* Date    : 2012-5-18
* Descript: io module public header.
* Update  : date                auther      ver     notes
*           2012-5-18 10:04:21  Sunny       1.0     Create this file.
*********************************************************************************************************
*/

#ifndef __REG_BASE_H__
#define __REG_BASE_H__

#define CCU_REG_BASE            (0x02001000)
#define CPUX_HWMSGBOX_REG_BASE  (0x03003000)
#define CPUS_HWMSGBOX_REG_BASE  (0x07094000)
#define RTC_REG_BASE            (0x07090000)
#define R_CPUCFG_REG_BASE       (0x07050000)
#define R_PRCM_REG_BASE         (0x07010000)
#define R_TMR01_REG_BASE        (0x07090400)
#define R_WDOG_REG_BASE         (0x07020400)
#define R_INTC_REG_BASE         (0x07021000)
#define R_PIO_REG_BASE          (0x07022000)
#define R_CIR_REG_BASE          (0x07040000)
#define R_UART_REG_BASE         (0x07080000)
#define R_TWI_REG_BASE          (0x07081400)
#define CPUSUBSYS_REG_BASE      (0x08000000)
#define TS_STAT_REG_BASE        (0x08010000)
#define TS_CTRL_REG_BASE        (0x08020000)
#define CPUCFG_REG_BASE         (0x08815000)

/* rtc domain record reg */
#define RTC_LOSC_CTRL           (RTC_REG_BASE + 0x000)
#define RTC_LOSC_OUT_GATING     (RTC_REG_BASE + 0x060)
#define RTC_RECORD_REG          (RTC_REG_BASE + 0x10c)
#define RTC_DTB_BASE_STORE_REG  (RTC_RECORD_REG)
#define RTC_XO_CTRL_REG         (RTC_REG_BASE + 0x160)

/*
 *  device physical addresses
 */
#define SUNXI_SID_PBASE         (0x03006000)
#define SUNXI_SMC_PBASE         (0x03110000)

/* prcm regs */
#define CPUS_CFG_REG                (R_PRCM_REG_BASE + 0x000)
#define AHBS_CFG_REG                (CPUS_CFG_REG)
#define APBS0_CFG_REG               (R_PRCM_REG_BASE + 0x00c)
#define APBS1_CFG_REG               (R_PRCM_REG_BASE + 0x010)
#define R_TIMER_BUS_GATE_RST_REG    (R_PRCM_REG_BASE + 0x11c)
#define R_PWM_BUS_GATE_RST_REG      (R_PRCM_REG_BASE + 0x13c)
#define R_MSGBOX_GATE_RST_REG       (R_PRCM_REG_BASE + 0x17c)
#define R_UART_BUS_GATE_RST_REG     (R_PRCM_REG_BASE + 0x18c)
#define R_TWI_BUS_GATE_RST_REG      (R_PRCM_REG_BASE + 0x19c)
#define R_IR_RX_CLOCK_REG           (R_PRCM_REG_BASE + 0x1c0)
#define R_IR_RX_BUS_GATE_RST_REG    (R_PRCM_REG_BASE + 0x1cc)
#define RTC_BUS_GATE_RST_REG        (R_PRCM_REG_BASE + 0x20c)
#define PLL_CTRL_REG1               (R_PRCM_REG_BASE + 0x244)
#define VDD_SYS_PWROFF_GATING_REG   (R_PRCM_REG_BASE + 0x250)
#define ANA_PWR_RST_REG             (R_PRCM_REG_BASE + 0x254)
#define VDD_SYS_PWR_RST_REG         (R_PRCM_REG_BASE + 0x260)
#define LP_CTRL_REG                 (R_PRCM_REG_BASE + 0x33c)
#define R_TIMER0_CLK_REG            (R_PRCM_REG_BASE + 0x100)

/* APBS0_CFG_REG */
#define APBS0_CLK_SRC_SEL_SHIFT (24)
#define APBS0_FACTOR_M_SHIFT (0)

#define APBS0_CLK_SRC_SEL_MASK (0x7 << APBS0_CLK_SRC_SEL_SHIFT)
#define APBS0_FACTOR_M_MASK (0x1f << APBS0_FACTOR_M_SHIFT)

#define APBS0_CLK_SRC_SEL(n) ((n) << APBS0_CLK_SRC_SEL_SHIFT)
#define APBS0_FACTOR_M(n) ((n) << APBS0_FACTOR_M_SHIFT)

/* APBS1_CFG_REG */
#define APBS1_CLK_SRC_SEL_SHIFT (24)
#define APBS1_FACTOR_M_SHIFT (0)

#define APBS1_CLK_SRC_SEL_MASK (0x7 << APBS1_CLK_SRC_SEL_SHIFT)
#define APBS1_FACTOR_M_MASK (0x1f << APBS1_FACTOR_M_SHIFT)

#define APBS1_CLK_SRC_SEL(n) ((n) << APBS1_CLK_SRC_SEL_SHIFT)
#define APBS1_FACTOR_M(n) ((n) << APBS1_FACTOR_M_SHIFT)

/* CPUS_CFG_REG */
#define CPUS_CLK_SRC_SEL_SHIFT (24)
#define CPUS_FACTOR_M_SHIFT (0)

#define CPUS_CLK_SRC_SEL_MASK (0x7 << CPUS_CLK_SRC_SEL_SHIFT)
#define CPUS_FACTOR_M_MASK (0x1f << CPUS_FACTOR_M_SHIFT)

#define CPUS_CLK_SRC_SEL(n) ((n) << CPUS_CLK_SRC_SEL_SHIFT)
#define CPUS_FACTOR_M(n) ((n) << CPUS_FACTOR_M_SHIFT)

//name by pll function
#define CCU_PLL_DDR0_REG            (CCU_REG_BASE + 0X010)
#define CCU_PLL_PERIPH0_REG         (CCU_REG_BASE + 0x020)
#define CCU_PLL_PERIPH1_REG         (CCU_REG_BASE + 0x028)
#define CCU_AHB_CLK_REG             (CCU_REG_BASE + 0x510)
#define CCU_APB0_CFG_REG            (CCU_REG_BASE + 0x520)
#define CCU_APB1_CFG_REG            (CCU_REG_BASE + 0x524)
#define CCU_MBUS_CLK_REG            (CCU_REG_BASE + 0x540)
#define CCU_MSGBOX_BGR_REG          (CCU_REG_BASE + 0x71c)
#define CCU_SPINLOCK_BGR_REG        (CCU_REG_BASE + 0x72c)

/* R_PIO_REG*/
#define PIN_CFG_REG(n, i)           ((volatile u32 *)(R_PIO_REG_BASE + ((n) - 1) * 0x30 + ((i) >> 3) * 0x4 + 0x00))
#define PIN_DLEVEL_REG(n, i)        ((volatile u32 *)(R_PIO_REG_BASE + ((n) - 1) * 0x30 + ((i) >> 3) * 0x4 + 0x14))
#define PIN_PULL_REG(n, i)          ((volatile u32 *)(R_PIO_REG_BASE + ((n) - 1) * 0x30 + ((i) >> 4) * 0x4 + 0x24))
#define PIN_DATA_REG(n)             ((volatile u32 *)(R_PIO_REG_BASE + ((n) - 1) * 0x30 + 0x10))

#define PIN_INT_CFG_REG(n, i)       ((volatile u32 *)(R_PIO_REG_BASE + (n - 1) * 0x20 + ((i) >> 3) * 0x4 + 0x200))
#define PIN_INT_CTL_REG(n)          ((volatile u32 *)(R_PIO_REG_BASE + (n - 1) * 0x20 + 0x210))
#define PIN_INT_STAT_REG(n)         ((volatile u32 *)(R_PIO_REG_BASE + (n - 1) * 0x20 + 0x214))
#define PIN_INT_DEBOUNCE_REG(n)     ((volatile u32 *)(R_PIO_REG_BASE + (n - 1) * 0x20 + 0x218))
#define PIN_INT_NUM_OFFSET(i)       (((i << 1) >> 1) * 0x4)

#define CCU_HOSC_FREQ               (24000000)	//24M
#define CCU_LOSC_FREQ               (31250)	//31250
#define CCU_CPUS_PLL0_FREQ          (200000000)
#define CCU_APBS2_PLL0_FREQ         (200000000)
#define CCU_IOSC_FREQ               (16000000)	//16M
#define CCU_CPUS_POST_DIV           (100000000)	//cpus post div source clock freq
#define CCU_PERIPH0_FREQ            (600000000)	//600M

#endif
