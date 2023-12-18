/* SPDX-License-Identifier: GPL-2.0+ */

#include <asm/arch/plat-sun60iw1p1/clock_autogen.h>

#define sunxi_ccm_reg CCMU_st
// #define pll1_cfg pll_cpu0_ctrl_reg
#define apb2_cfg apb1_clk_reg
#define uart_gate_reset uart0_bgr_reg
// #define cpu_axi_cfg cpu_clk_reg
#define pll6_cfg pll_peri0_ctrl_reg
#define psi_ahb1_ahb2_cfg ahb_clk_reg
#define apb1_cfg apb0_clk_reg
#define mbus_cfg mbus_clk_reg
// #define ve_clk_cfg ve_clk_reg
#define de_clk_cfg de0_clk_reg
#define mbus_gate mbus_mat_clk_gating_reg
#define dma_gate_reset dma0_bgr_reg
#define sd0_clk_cfg smhc0_clk_reg
#define sd1_clk_cfg smhc1_clk_reg
#define sd2_clk_cfg smhc2_clk_reg
#define sd3_clk_cfg smhc3_clk_reg
#define sd_gate_reset smhc0_bgr_reg
#define twi_gate_reset twi0_bgr_reg
#define ce_gate_reset ce_bgr_reg
#define ce_clk_cfg ce_clk_reg

#define APB2_CLK_SRC_OSC24M                                                    \
	(APB1_CLK_REG_CLK_SRC_SEL_HOSC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define APB2_CLK_SRC_OSC32K                                                    \
	(APB2_CLK_SRC_OSC32K << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define APB2_CLK_SRC_PSI                                                       \
	(APB1_CLK_REG_CLK_SRC_SEL_CLK16M_RC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define APB2_CLK_SRC_PLL6                                                      \
	(APB1_CLK_REG_CLK_SRC_SEL_PERI0_600M_BUS                               \
	 << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)

#define APB2_CLK_RATE_N_1 (0x0 << 8)
#define APB2_CLK_RATE_N_2 (0x1 << 8)
#define APB2_CLK_RATE_N_4 (0x2 << 8)
#define APB2_CLK_RATE_N_8 (0x3 << 8)
#define APB2_CLK_RATE_N_MASK (3 << 8)
#define APB2_CLK_RATE_M(m) (((m)-1) << APB1_CLK_REG_FACTOR_M_OFFSET)
#define APB2_CLK_RATE_M_MASK (3 << APB1_CLK_REG_FACTOR_M_OFFSET)

/* MMC clock bit field */
#define CCM_MMC_CTRL_M(x) ((x)-1)
#define CCM_MMC_CTRL_N(x) ((x) << SMHC0_CLK_REG_FACTOR_N_OFFSET)
#define CCM_MMC_CTRL_OSCM24                                                    \
	(SMHC0_CLK_REG_CLK_SRC_SEL_HOSC << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL6X2                                                    \
	(SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_400M                                  \
	 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL_PERIPH2X2                                             \
	(SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_300M                                  \
	 << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_ENABLE                                                    \
	(SMHC0_CLK_REG_SMHC0_CLK_GATING_CLOCK_IS_ON                            \
	 << SMHC0_CLK_REG_SMHC0_CLK_GATING_OFFSET)
/* if doesn't have these delays */
#define CCM_MMC_CTRL_OCLK_DLY(a) ((void)(a), 0)
#define CCM_MMC_CTRL_SCLK_DLY(a) ((void)(a), 0)

/* Module gate/reset shift*/
#define RESET_SHIFT (16)
#define GATING_SHIFT (0)

/*CE*/
#define CE_CLK_SRC_MASK (0x1)
#define CE_CLK_SRC (0x01)

#define CE_CLK_DIV_RATION_N_BIT (8)
#define CE_CLK_DIV_RATION_N_MASK (0x3)
#define CE_CLK_DIV_RATION_N (0)

#define CE_CLK_DIV_RATION_M_MASK (0xF)
#define CE_CLK_DIV_RATION_M (2)

#define CE_MBUS_GATING_MASK (1)
#define CE_MBUS_GATING_BIT (MBUS_MAT_CLK_GATING_REG_CE_MCLK_EN_OFFSET)
#define CE_MBUS_GATING (MBUS_MAT_CLK_GATING_REG_CE_MCLK_EN_OFFSET)

/* timer*/
#define CCMU_TIMER0_BGR_REG  (SUNXI_CCM_BASE + TIMER0_BGR_REG)
#define CCMU_TIMER1_BGR_REG  (SUNXI_CCM_BASE + TIMER1_BGR_REG)

/* pll list */
#define CCMU_PLL_CPUX_CTRL_REG (SUNXI_CCM_BASE + 0x00)
#define CCMU_PLL_DDR0_CTRL_REG (SUNXI_CCM_BASE + 0x10)
#define CCMU_PLL_DDR1_CTRL_REG (SUNXI_CCM_BASE + 0x18)
#define CCMU_PLL_PERI0_CTRL_REG (SUNXI_CCM_BASE + 0x20)
#define CCMU_PLL_PERI1_CTRL_REG (SUNXI_CCM_BASE + 0x28)
#define CCMU_PLL_GPU_CTRL_REG (SUNXI_CCM_BASE + 0x30)
#define CCMU_PLL_VIDE00_CTRL_REG (SUNXI_CCM_BASE + 0x40)
#define CCMU_PLL_VIDE01_CTRL_REG (SUNXI_CCM_BASE + 0x48)
#define CCMU_PLL_VIDE02_CTRL_REG (SUNXI_CCM_BASE + 0x50)
#define CCMU_PLL_VIDE03_CTRL_REG (SUNXI_CCM_BASE + 0x68)
#define CCMU_PLL_VE_CTRL_REG (SUNXI_CCM_BASE + 0x58)
#define CCMU_PLL_COM_CTRL_REG (SUNXI_CCM_BASE + 0x60)
#define CCMU_PLL_AUDIO_CTRL_REG (SUNXI_CCM_BASE + 0x78)

#define CCMU_PLL_HSIC_CTRL_REG (SUNXI_CCM_BASE + 0x70)

/* cfg list */
#define CCMU_CPUX_AXI_CFG_REG (SUNXI_CCM_BASE + 0x500)
#define CCMU_PSI_AHB1_AHB2_CFG_REG (SUNXI_CCM_BASE + 0x510)
#define CCMU_AHB3_CFG_GREG (SUNXI_CCM_BASE + 0x51C)
#define CCMU_APB1_CFG_GREG (SUNXI_CCM_BASE + 0x520)
#define CCMU_APB2_CFG_GREG (SUNXI_CCM_BASE + 0x524)
#define CCMU_MBUS_CFG_REG (SUNXI_CCM_BASE + 0x540)

#define CCMU_CE_CLK_REG (SUNXI_CCM_BASE + 0x680)
#define CCMU_CE_BGR_REG (SUNXI_CCM_BASE + 0x68C)

#define CCMU_VE_CLK_REG (SUNXI_CCM_BASE + 0x690)
#define CCMU_VE_BGR_REG (SUNXI_CCM_BASE + 0x69C)

/*SYS*/
#define CCMU_DMA_BGR_REG (SUNXI_CCM_BASE + 0x70C)
#define CCMU_AVS_CLK_REG (SUNXI_CCM_BASE + 0x750)
#define CCMU_AVS_BGR_REG (SUNXI_CCM_BASE + 0x74C)

/*IOMMU*/
#define CCMU_IOMMU_BGR_REG (SUNXI_CCM_BASE + 0x7bc)
#define IOMMU_AUTO_GATING_REG (SUNXI_IOMMU_BASE + 0X40)

/* storage */
#define CCMU_DRAM_CLK_REG (SUNXI_CCM_BASE + 0x800)
#define CCMU_MBUS_MST_CLK_GATING_REG (SUNXI_CCM_BASE + 0x804)
#define CCMU_PLL_DDR_AUX_REG (SUNXI_CCM_BASE + 0x808)
#define CCMU_DRAM_BGR_REG (SUNXI_CCM_BASE + 0x80C)

#define CCMU_NAND_CLK_REG (SUNXI_CCM_BASE + 0x810)
#define CCMU_NAND_BGR_REG (SUNXI_CCM_BASE + 0x82C)

#define CCMU_SDMMC0_CLK_REG (SUNXI_CCM_BASE + SMHC0_CLK_REG)
#define CCMU_SDMMC1_CLK_REG (SUNXI_CCM_BASE + SMHC1_CLK_REG)
#define CCMU_SDMMC2_CLK_REG (SUNXI_CCM_BASE + SMHC2_CLK_REG)
#define CCMU_SMHC0_BGR_REG (SUNXI_CCM_BASE + SMHC0_BGR_REG)

/*normal interface*/
#define CCMU_UART_BGR_REG (SUNXI_CCM_BASE + 0x90C)
#define CCMU_TWI_BGR_REG (SUNXI_CCM_BASE + 0x91C)

#define CCMU_SCR_BGR_REG (SUNXI_CCM_BASE + 0x93C)

#define CCMU_SPI0_CLK_REG (SUNXI_CCM_BASE + 0x940)
#define CCMU_SPI1_CLK_REG (SUNXI_CCM_BASE + 0x944)
#define CCMU_SPI_BGR_CLK_REG (SUNXI_CCM_BASE + 0x96C)
#define CCMU_USB0_CLK_REG (SUNXI_CCM_BASE + 0xA70)
#define CCMU_USB_BGR_REG (SUNXI_CCM_BASE + 0xA8C)

/*DMA*/
#define DMA_GATING_BASE CCMU_DMA_BGR_REG
#define DMA_GATING_PASS (1)
#define DMA_GATING_BIT (0)

/*CE*/
#define CE_CLK_SRC_MASK (0x1)
#define CE_CLK_SRC_SEL_BIT (24)
#define CE_CLK_SRC (0x01)

#define CE_CLK_DIV_RATION_N_BIT (8)
#define CE_CLK_DIV_RATION_N_MASK (0x3)
#define CE_CLK_DIV_RATION_N (0)

#define CE_CLK_DIV_RATION_M_BIT (0)
#define CE_CLK_DIV_RATION_M_MASK (0xF)
#define CE_CLK_DIV_RATION_M (2)

#define CE_SCLK_ONOFF_BIT (31)
#define CE_SCLK_ON (1)

#define CE_GATING_BASE CCMU_CE_BGR_REG
#define CE_GATING_PASS (1)
#define CE_GATING_BIT (0)

#define CE_RST_REG_BASE CCMU_CE_BGR_REG
#define CE_RST_BIT (16)
#define CE_DEASSERT (1)

/*gpadc gate and reset reg*/
#define CCMU_GPADC_BGR_REG (SUNXI_CCM_BASE + 0x09EC)

/*lpadc gate and reset reg*/
#define CCMU_LRADC_BGR_REG (SUNXI_CCM_BASE + 0x0A9C)

/* ehci */
#define BUS_CLK_GATING_REG 0x60
#define BUS_SOFTWARE_RESET_REG 0x2c0
#define USBPHY_CONFIG_REG 0xcc

#define USBEHCI0_RST_BIT 24
#define USBEHCI0_GATIING_BIT 24
#define USBPHY0_RST_BIT 0
#define USBPHY0_SCLK_GATING_BIT 8

#define USBEHCI1_RST_BIT 25
#define USBEHCI1_GATIING_BIT 25
#define USBPHY1_RST_BIT 1
#define USBPHY1_SCLK_GATING_BIT 9
