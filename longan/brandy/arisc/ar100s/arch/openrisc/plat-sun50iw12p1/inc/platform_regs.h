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
#define HWMSGBOX_REG_BASE       (0x03003000)
#define CPUX_HWMSGBOX_REG_BASE  (HWMSGBOX_REG_BASE)
#define CPUS_HWMSGBOX_REG_BASE  (HWMSGBOX_REG_BASE + 0x400)
#define RTC_REG_BASE            (0x07090000)
#define R_CPUCFG_REG_BASE       (0x07000400)
#define R_PRCM_REG_BASE         (0x07010000)
#define R_TMR01_REG_BASE        (0x07090400)
#define R_WDOG_REG_BASE         (0x07020400)
#define R_INTC_REG_BASE         (0x07021000)
#define R_PIO_REG_BASE          (0x07022000)
#define R_CIR_REG_BASE          (0x07040000)
#define R_UART_REG_BASE         (0x07080000)
#define R_TWI_REG_BASE          (0x07081400)
#define CPUSUBSYS_REG_BASE      (0x08100000)
#define TS_STAT_REG_BASE        (0x08110000)
#define TS_CTRL_REG_BASE        (0x08120000)
#define CPUCFG_REG_BASE         (0x09010000)
#define TVFE_TOP_BASE           (0x05700000)

//rtc domain record reg
#define RTC_LOSC_CTRL           (RTC_REG_BASE + 0x000)
#define RTC_LOSC_OUT_GATING     (RTC_REG_BASE + 0x060)
#define RTC_IR_CODE_STORE_REG   (RTC_REG_BASE + 0x100)
#define RTC_FAKE_POWER_OFF_REG  (RTC_REG_BASE + 0x108)
#define RTC_RECORD_REG          (RTC_REG_BASE + 0x10c)
#define RTC_DTB_BASE_STORE_REG  (RTC_RECORD_REG)
#define RTC_XO_CTRL_REG         (RTC_REG_BASE + 0x160)

/*
 *  device physical addresses  \\not use now
 */
#define SUNXI_SID_PBASE                  0x03006000
#define SUNXI_SMC_PBASE                  0x04800000

//prcm regs
#define CPUS_CFG_REG                (R_PRCM_REG_BASE + 0x000)
#define APBS1_CFG_REG               (R_PRCM_REG_BASE + 0x00c)
#define APBS2_CFG_REG               (R_PRCM_REG_BASE + 0x010)
#define R_TIMER_BUS_GATE_RST_REG    (R_PRCM_REG_BASE + 0x11c)
#define EDID_CFG_REG                (R_PRCM_REG_BASE + 0x124)
#define R_PWM_BUS_GATE_RST_REG      (R_PRCM_REG_BASE + 0x13c)
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
#define LDO_CTRL_REG                (R_PRCM_REG_BASE + 0x340)

#define R_TIMER0_CLK_REG            (R_PRCM_REG_BASE + 0x110)

/* VDD_SYS_PWR_RST_REG */
#define VDD_SYS_MODULE_RST_SHIFT (0)
#define VDD_SYS_MODULE_RST_MASK (0x1 << VDD_SYS_MODULE_RST_SHIFT)
#define VDD_SYS_MODULE_RST(n) ((n) << VDD_SYS_MODULE_RST_SHIFT)

/* ANA_PWR_RST_REG */
#define AVCC_A_GATING_SHIFT (1)
#define RES_VDD_ON_CTRL_SHIFT (0)

#define AVCC_A_GATING_MASK (0x1 << AVCC_A_GATING_SHIFT)
#define RES_VDD_ON_CTRL_MASK (0x1 << RES_VDD_ON_CTRL_SHIFT)

#define AVCC_A_GATING(n) ((n) << AVCC_A_GATING_SHIFT)
#define RES_VDD_ON_CTRL(n) ((n) << RES_VDD_ON_CTRL_SHIFT)

/* VDD_SYS_PWROFF_GATING_REG */
#define VDD_USB2CPUS_GATING_SHIFT (8)
#define VDD_SYS2USB_GATING_SHIFT (3)
#define VDD_CPUS_GATING_SHIFT (2)

#define VDD_USB2CPUS_GATING_MASK (0x1 << VDD_USB2CPUS_GATING_SHIFT)
#define VDD_SYS2USB_GATING_MASK (0x1 << VDD_SYS2USB_GATING_SHIFT)
#define VDD_CPUS_GATING_MASK (0x1 << VDD_CPUS_GATING_SHIFT)

#define VDD_USB2CPUS_GATING(n) ((n) << VDD_USB2CPUS_GATING_SHIFT)
#define VDD_SYS2USB_GATING(n) ((n) << VDD_SYS2USB_GATING_SHIFT)
#define VDD_CPUS_GATING(n) ((n) << VDD_CPUS_GATING_SHIFT)

/* APBS1_CFG_REG */
#define APBS2_CLK_SRC_SEL_SHIFT (24)
#define APBS2_CLK_DIV_RATIO_N_SHIFT (8)
#define APBS2_FACTOR_M_SHIFT (0)

#define APBS2_CLK_SRC_SEL_MASK (0x7 << APBS2_CLK_SRC_SEL_SHIFT)
#define APBS2_CLK_DIV_RATIO_N_MASK (0x3 << APBS2_CLK_DIV_RATIO_N_SHIFT)
#define APBS2_FACTOR_M_MASK (0x1f << APBS2_FACTOR_M_SHIFT)

#define APBS2_CLK_SRC_SEL(n) ((n) << APBS2_CLK_SRC_SEL_SHIFT)
#define APBS2_CLK_DIV_RATIO_N(n) ((n) << APBS2_CLK_DIV_RATIO_N_SHIFT)
#define APBS2_FACTOR_M(n) ((n) << APBS2_FACTOR_M_SHIFT)

/* CPUS_CFG_REG */
#define CPUS_CLK_SRC_SEL_SHIFT (24)
#define CPUS_CLK_DIV_N_SHIFT (8)
#define CPUS_FACTOR_M_SHIFT (0)

#define CPUS_CLK_SRC_SEL_MASK (0x7 << CPUS_CLK_SRC_SEL_SHIFT)
#define CPUS_CLK_DIV_N_MASK (0x3 << CPUS_CLK_DIV_N_SHIFT)
#define CPUS_FACTOR_M_MASK (0x1f << CPUS_FACTOR_M_SHIFT)

#define CPUS_CLK_SRC_SEL(n) ((n) << CPUS_CLK_SRC_SEL_SHIFT)
#define CPUS_CLK_DIV_N(n) ((n) << CPUS_CLK_DIV_N_SHIFT)
#define CPUS_FACTOR_M(n) ((n) << CPUS_FACTOR_M_SHIFT)


#define APBS1_CLK_SRC_SEL_SHIFT (24)
#define APBS1_CLK_DIV_N_SHIFT (8)
#define APBS1_FACTOR_M_SHIFT (0)

#define APBS1_CLK_SRC_SEL_MASK (0x7 << APBS1_CLK_SRC_SEL_SHIFT)
#define APBS1_CLK_DIV_N_MASK (0x3 << APBS1_CLK_DIV_N_SHIFT)
#define APBS1_FACTOR_M_MASK (0x1f << APBS1_FACTOR_M_SHIFT)

#define APBS1_CLK_SRC_SEL(n) ((n) << APBS1_CLK_SRC_SEL_SHIFT)
#define APBS1_CLK_DIV_N(n) ((n) << APBS1_CLK_DIV_N_SHIFT)
#define APBS1_FACTOR_M(n) ((n) << APBS1_FACTOR_M_SHIFT)

//name by pll order
#define CCU_PLLx_REG(n)             (CCU_REG_BASE + (0x8 * (n - 1)))

//name by pll function
#define CCU_PLL_C0_REG              (CCU_REG_BASE + 0x000)
#define CCU_PLL_DDR0_REG            (CCU_REG_BASE + 0X010)
#define CCU_PLL_PERIPH0_REG         (CCU_REG_BASE + 0x020)
#define CCU_PLL_PERIPH1_REG         (CCU_REG_BASE + 0x028)
#define CCU_PLL_GPU0_CTRL_REG       (CCU_REG_BASE + 0x030)
#define CCU_PLL_VIDEO0_CTRL_REG     (CCU_REG_BASE + 0x040)
#define CCU_PLL_VIDEO1_CTRL_REG     (CCU_REG_BASE + 0x048)
#define CCU_PLL_VIDEO2_CTRL_REG     (CCU_REG_BASE + 0x050)
#define CCU_PLL_VE_CTRL_REG         (CCU_REG_BASE + 0x058)
#define CCU_PLL_ADC_CTRL_REG        (CCU_REG_BASE + 0x060)
#define CCU_PLL_VIDEO3_CTRL_REG     (CCU_REG_BASE + 0x068)
#define CCU_PLL_AUDIO0_CTRL_REG     (CCU_REG_BASE + 0x078)
#define CCU_PLL_AUDIO1_CTRL_REG     (CCU_REG_BASE + 0x080)
#define CCU_CPU_AXI_CFG_REG         (CCU_REG_BASE + 0x500)
#define CCU_PSI_CFG_REG             (CCU_REG_BASE + 0x510)
#define CCU_APB1_CFG_REG            (CCU_REG_BASE + 0x520)
#define CCU_APB2_CFG_REG            (CCU_REG_BASE + 0x524)
#define CCU_MBUS_CLK_REG            (CCU_REG_BASE + 0x540)
#define CCU_MSGBOX_BGR_REG          (CCU_REG_BASE + 0x71c)
#define CCU_SPINLOCK_BGR_REG        (CCU_REG_BASE + 0x72c)
#define CCU_USB0_CLOCK_REG          (CCU_REG_BASE + 0xA70)
#define CCU_USB_BUS_GATING_RST_REG  (CCU_REG_BASE + 0xA8C)
#define CCMU_DISP_BGR_REG           (CCU_REG_BASE + 0xDD8)

//TVFE TOP REG
#define SVP_CLK_GATE_REG		(TVFE_TOP_BASE + 0x40)
#define SVP_RST_GATE_REG		(TVFE_TOP_BASE + 0x44)
#define PNL_CLK_GATE_REG		(TVFE_TOP_BASE + 0x80)

/* MSGBOX_REG */
#define MSGBOX_VER_REG(m, n)		(HWMSGBOX_REG_BASE + 0x10 + m*0x400 + n*0x100)
#define MSGBOX_MSG_DEBUG_REG(m, n)		(HWMSGBOX_REG_BASE + 0x40 + m*0x400 + n*0x100)
#define MSGBOX_FIFO_STA_REG(m, n, c)	(HWMSGBOX_REG_BASE + 0x50 + m*0x400 + n*0x100 + c*0x4)
#define MSGBOX_MSG_STA_REG(m, n, c)	(HWMSGBOX_REG_BASE + 0x60 + m*0x400 + n*0x100 + c*0x4)
#define MSGBOX_MESG_REG(m, n, c)	(HWMSGBOX_REG_BASE + 0x70 + m*0x400 + n*0x100 + c*0x4)

/* R_PIO_REG*/
#define PIN_CFG_REG(n, i)           ((volatile u32 *)(R_PIO_REG_BASE + ((n) - 1) * 0x24 + ((i) >> 3) * 0x4 + 0x00))
#define PIN_DLEVEL_REG(n, i)        ((volatile u32 *)(R_PIO_REG_BASE + ((n) - 1) * 0x24 + ((i) >> 4) * 0x4 + 0x14))
#define PIN_PULL_REG(n, i)          ((volatile u32 *)(R_PIO_REG_BASE + ((n) - 1) * 0x24 + ((i) >> 4) * 0x4 + 0x1C))
#define PIN_DATA_REG(n)             ((volatile u32 *)(R_PIO_REG_BASE + ((n) - 1) * 0x24 + 0x10))

#define PIN_INT_CFG_REG(n, i)       ((volatile u32 *)(R_PIO_REG_BASE + (n - 1) * 0x20 + ((i) >> 3) * 0x4 + 0x200))
#define PIN_INT_CTL_REG(n)          ((volatile u32 *)(R_PIO_REG_BASE + (n - 1) * 0x20 + 0x210))
#define PIN_INT_STAT_REG(n)         ((volatile u32 *)(R_PIO_REG_BASE + (n - 1) * 0x20 + 0x214))
#define PIN_INT_DEBOUNCE_REG(n)     ((volatile u32 *)(R_PIO_REG_BASE + (n - 1) * 0x20 + 0x218))
#define PIN_INT_NUM_OFFSET(i)       (((i << 1) >> 1) * 0x4)

/*smc config info*/
#define SMC_ACTION_REG                (SUNXI_SMC_PBASE + 0x0004)
#define SMC_MST0_BYP_REG              (SUNXI_SMC_PBASE + 0x0070)
#define SMC_MST1_BYP_REG              (SUNXI_SMC_PBASE + 0x0074)
#define SMC_MST2_BYP_REG              (SUNXI_SMC_PBASE + 0x0078)

#define SMC_MST0_SEC_REG              (SUNXI_SMC_PBASE + 0x0080)
#define SMC_MST1_SEC_REG              (SUNXI_SMC_PBASE + 0x0084)
#define SMC_MST2_SEC_REG              (SUNXI_SMC_PBASE + 0x0088)

#define SMC_REGION_COUNT (8) /*total 16,not all used, save some space*/
#define SMC_REGIN_SETUP_LOW_REG(x)    (SUNXI_SMC_PBASE + 0x100 + 0x10*(x))
#define SMC_REGIN_SETUP_HIGH_REG(x)   (SUNXI_SMC_PBASE + 0x104 + 0x10*(x))
#define SMC_REGIN_ATTRIBUTE_REG(x)    (SUNXI_SMC_PBASE + 0x108 + 0x10*(x))

/*SID*/
#define SID_SEC_MODE_STA              (SUNXI_SID_PBASE + 0xA0)
#define SID_SEC_MODE_MASK             (0x1)

/* CCU_USB_BUS_GATING_RST_REG */
#define PLL_NUM     (14)
#define BUS_NUM     (10)
#define IO_NUM      (2)

#define CCU_HOSC_FREQ               (24000000)	//24M
#define CCU_LOSC_FREQ               (31250)	//31250
#define CCU_CPUS_PLL0_FREQ          (200000000)
#define CCU_APBS2_PLL0_FREQ         (200000000)
#define CCU_IOSC_FREQ               (16000000)	//16M
#define CCU_CPUS_POST_DIV           (100000000)	//cpus post div source clock freq
#define CCU_PERIPH0_FREQ            (600000000)	//600M

/* CCU_PLL_C0_REG */
#define CPUX_PLL_ENABLE_SHIFT (31)

#define CPUX_PLL_ENABLE_MASK (0x1 << CPUX_PLL_ENABLE_SHIFT)

#define CPUX_PLL_ENABLE(n) ((n) << CPUX_PLL_ENABLE_SHIFT)

/* CCU_PLL_DDR0_REG */
#define DDR0_PLL_ENABLE_SHIFT (31)

#define DDR0_PLL_ENABLE_MASK (0x1 << DDR0_PLL_ENABLE_SHIFT)

#define DDR0_PLL_ENABLE(n) ((n) << DDR0_PLL_ENABLE_SHIFT)

/* CCU_PLL_PERIPH0_REG */
#define PERIPH0_PLL_ENABLE_SHIFT (31)

#define PERIPH0_PLL_ENABLE_MASK (0x1 << PERIPH0_PLL_ENABLE_SHIFT)

#define PERIPH0_PLL_ENABLE(n) ((n) << PERIPH0_PLL_ENABLE_SHIFT)

/* CCU_PLL_PERIPH1_REG */
#define PERIPH1_PLL_ENABLE_SHIFT (31)

#define PERIPH1_PLL_ENABLE_MASK (0x1 << PERIPH1_PLL_ENABLE_SHIFT)

#define PERIPH1_PLL_ENABLE(n) ((n) << PERIPH1_PLL_ENABLE_SHIFT)

/* CCU_CPU_AXI_CFG_REG */
#define CPUX_CLK_SRC_SEL_SHIFT (24)
#define CPUX_APB_FACTOR_N_SHIFT (8)
#define CPUX_AXI_FACTOR_M_SHIFT (0)

#define CPUX_CLK_SRC_SEL_MASK (0x7 << CPUX_CLK_SRC_SEL_SHIFT)
#define CPUX_APB_FACTOR_N_MASK (0x3 << CPUX_APB_FACTOR_N_SHIFT)
#define CPUX_AXI_FACTOR_M_MASK (0x3 << CPUX_AXI_FACTOR_M_SHIFT)

#define CPUX_CLK_SRC_SEL(n) ((n) << CPUX_CLK_SRC_SEL_SHIFT)
#define CPUX_APB_FACTOR_N(n) ((n) << CPUX_APB_FACTOR_N_SHIFT)
#define CPUX_AXI_FACTOR_M(n) ((n) << CPUX_AXI_FACTOR_M_SHIFT)

/* CCU_PSI_AHB1_AHB2_CFG_REG */
#define PSI_CLK_SRC_SEL_SHIFT (24)
#define PSI_FACTOR_N_SHIFT (8)
#define PSI_FACTOR_M_SHIFT (0)

#define PSI_CLK_SRC_SEL_MASK (0x7 << PSI_CLK_SRC_SEL_SHIFT)
#define PSI_FACTOR_N_MASK (0x3 << PSI_FACTOR_N_SHIFT)
#define PSI_FACTOR_M_MASK (0x3 << PSI_FACTOR_M_SHIFT)

#define PSI_CLK_SRC_SEL(n) ((n) << PSI_CLK_SRC_SEL_SHIFT)
#define PSI_FACTOR_N(n) ((n) << PSI_FACTOR_N_SHIFT)
#define PSI_FACTOR_M(n) ((n) << PSI_FACTOR_M_SHIFT)

/* CCU_APB1_CFG_REG */
#define APB1_CLK_SRC_SEL_SHIFT (24)
#define APB1_FACTOR_N_SHIFT (8)
#define APB1_FACTOR_M_SHIFT (0)

#define APB1_CLK_SRC_SEL_MASK (0x7 << APB1_CLK_SRC_SEL_SHIFT)
#define APB1_FACTOR_N_MASK (0x3 << APB1_FACTOR_N_SHIFT)
#define APB1_FACTOR_M_MASK (0x3 << APB1_FACTOR_M_SHIFT)

#define APB1_CLK_SRC_SEL(n) ((n) << APB1_CLK_SRC_SEL_SHIFT)
#define APB1_FACTOR_N(n) ((n) << APB1_FACTOR_N_SHIFT)
#define APB1_FACTOR_M(n) ((n) << APB1_FACTOR_M_SHIFT)

/* CCU_APB2_CFG_REG */
#define APB2_CLK_SRC_SEL_SHIFT (24)
#define APB2_FACTOR_N_SHIFT (8)
#define APB2_FACTOR_M_SHIFT (0)

#define APB2_CLK_SRC_SEL_MASK (0x7 << APB2_CLK_SRC_SEL_SHIFT)
#define APB2_FACTOR_N_MASK (0x3 << APB2_FACTOR_N_SHIFT)
#define APB2_FACTOR_M_MASK (0x3 << APB2_FACTOR_M_SHIFT)

#define APB2_CLK_SRC_SEL(n) ((n) << APB2_CLK_SRC_SEL_SHIFT)
#define APB2_FACTOR_N(n) ((n) << APB2_FACTOR_N_SHIFT)
#define APB2_FACTOR_M(n) ((n) << APB2_FACTOR_M_SHIFT)

/* CCU_PLL_C0_REG */
#define C0_PLL_FACTOR_M_SHIFT			(0)
#define C0_PLL_FACTOR_N_SHIFT			(8)
#define C0_PLL_OUT_EXT_DIVP_SHIFT		(16)
#define C0_PLL_OUTPUT_ENABLE_SHIFT		(27)
#define C0_PLL_LOCK_STATUS_SHIFT		(28)
#define C0_PLL_LOCK_ENABLE_SHIFT		(29)
#define C0_PLL_ENABLE_SHIFT			(31)

#define C0_PLL_FACTOR_M_MASK			(0x3 << C0_PLL_FACTOR_M_SHIFT)
#define C0_PLL_FACTOR_N_MASK			(0xff << C0_PLL_FACTOR_N_SHIFT)
#define C0_PLL_OUT_EXT_DIVP_MASK		(0x3 << C0_PLL_OUT_EXT_DIVP_SHIFT)
#define C0_PLL_OUTPUT_ENABLE_MASK		(0x1 << C0_PLL_OUTPUT_ENABLE_SHIFT)
#define C0_PLL_LOCK_STATUS_MASK			(0x1 << C0_PLL_LOCK_STATUS_SHIFT)
#define C0_PLL_LOCK_ENABLE_MASK			(0x1 << C0_PLL_LOCK_ENABLE_SHIFT)
#define C0_PLL_ENABLE_MASK			(0x1 << C0_PLL_ENABLE_SHIFT)

#define C0_PLL_FACTOR_M(n)			((n) << C0_PLL_FACTOR_M_SHIFT)
#define C0_PLL_FACTOR_N(n)			((n) << C0_PLL_FACTOR_N_SHIFT)
#define C0_PLL_OUT_EXT_DIVP(n)			((n) << C0_PLL_OUT_EXT_DIVP_SHIFT)
#define C0_PLL_OUTPUT_ENABLE(n)			((n) << C0_PLL_OUTPUT_ENABLE_SHIFT)
#define C0_PLL_LOCK_STATUS(n)			((n) << C0_PLL_LOCK_STATUS_SHIFT)
#define C0_PLL_LOCK_ENABLE(n)			((n) << C0_PLL_LOCK_ENABLE_SHIFT)
#define C0_PLL_ENABLE(n)			((n) << C0_PLL_ENABLE_SHIFT)

#endif
