#include <config.h>
#include <arch/cpu.h>
#include <arch/sun55iw3p1/clock_autogen.h>
#define  sunxi_ccm_reg CCMU_st
#define  pll1_cfg                pll_cpu0_ctrl_reg
#define  apb2_cfg                apb1_clk_reg
#define  uart_gate_reset         uart_bgr_reg
#define  cpu_axi_cfg             cpu_clk_reg
#define  pll6_cfg                pll_peri0_ctrl_reg
#define  psi_ahb1_ahb2_cfg       ahb_clk_reg
#define  apb1_cfg                apb0_clk_reg
#define  mbus_cfg                mbus_clk_reg
#define  ve_clk_cfg              ve_clk_reg
#define  de_clk_cfg              de0_clk_reg
#define  mbus_gate               mbus_mat_clk_gating_reg
#define  dma_gate_reset          dma_bgr_reg
#define  sd0_clk_cfg             smhc0_clk_reg
#define  sd1_clk_cfg             smhc1_clk_reg
#define  sd2_clk_cfg             smhc2_clk_reg
#define  sd_gate_reset           smhc_bgr_reg
#define  twi_gate_reset          twi_bgr_reg
#define  ce_gate_reset           ce_bgr_reg
#define  ce_clk_cfg              ce_clk_reg

#define  APB2_CLK_SRC_OSC24M     (APB1_CLK_REG_CLK_SRC_SEL_HOSC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_OSC32K     (APB2_CLK_SRC_OSC32K << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_PSI        (APB1_CLK_REG_CLK_SRC_SEL_CLK16M_RC << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)
#define  APB2_CLK_SRC_PLL6       (APB1_CLK_REG_CLK_SRC_SEL_PERI0_600M_BUS << APB1_CLK_REG_CLK_SRC_SEL_OFFSET)

#define APB2_CLK_RATE_N_1               (0x0 << 8)
#define APB2_CLK_RATE_N_2               (0x1 << 8)
#define APB2_CLK_RATE_N_4               (0x2 << 8)
#define APB2_CLK_RATE_N_8               (0x3 << 8)
#define APB2_CLK_RATE_N_MASK            (3 << 8)
#define APB2_CLK_RATE_M(m)              (((m)-1) << APB1_CLK_REG_FACTOR_M_OFFSET)
#define APB2_CLK_RATE_M_MASK            (3 << APB1_CLK_REG_FACTOR_M_OFFSET)

/* MMC clock bit field */
#define CCM_MMC_CTRL_M(x)               ((x) - 1)
#define CCM_MMC_CTRL_N(x)               ((x) << SMHC0_CLK_REG_FACTOR_N_OFFSET)
#define CCM_MMC_CTRL_OSCM24             (SMHC0_CLK_REG_CLK_SRC_SEL_HOSC << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL6X2             (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_400M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_PLL_PERIPH2X2      (SMHC0_CLK_REG_CLK_SRC_SEL_PERI0_300M << SMHC0_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CCM_MMC_CTRL_ENABLE             (SMHC0_CLK_REG_SMHC0_CLK_GATING_CLOCK_IS_ON << SMHC0_CLK_REG_SMHC0_CLK_GATING_OFFSET)
/* if doesn't have these delays */
#define CCM_MMC_CTRL_OCLK_DLY(a)        ((void) (a), 0)
#define CCM_MMC_CTRL_SCLK_DLY(a)        ((void) (a), 0)

/* Module gate/reset shift*/
#define RESET_SHIFT                     (16)
#define GATING_SHIFT                    (0)

/* pll list */
#define CCMU_PLL_CPU0_CTRL_REG            (SUNXI_CPU_SYS_CFG_BASE + 0x817000)
#define CCMU_PLL_CPU1_CTRL_REG            (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x04)
#define CCMU_PLL_CPU2_CTRL_REG            (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x08)
#define CCMU_PLL_CPU3_CTRL_REG            (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x0c)
#define CCMU_PLL_DDR0_CTRL_REG            (SUNXI_CCM_BASE + PLL_DDR_CTRL_REG)
#define CCMU_PLL_DDR1_CTRL_REG            (SUNXI_CCM_BASE + 0x18)
#define CCMU_PLL_PERI0_CTRL_REG            (SUNXI_CCM_BASE + PLL_PERI0_CTRL_REG)
#define CCMU_PLL_PERI1_CTRL_REG         (SUNXI_CCM_BASE + PLL_PERI1_CTRL_REG)
#define CCMU_PLL_GPU_CTRL_REG                   (SUNXI_CCM_BASE + PLL_GPU_CTRL_REG)
#define CCMU_PLL_VIDE00_CTRL_REG                        (SUNXI_CCM_BASE + PLL_VIDEO0_CTRL_REG)
#define CCMU_PLL_VIDE01_CTRL_REG                        (SUNXI_CCM_BASE + PLL_VIDEO1_CTRL_REG)
#define CCMU_PLL_VIDE02_CTRL_REG                        (SUNXI_CCM_BASE + PLL_VIDEO2_CTRL_REG)
#define CCMU_PLL_VIDE03_CTRL_REG                        (SUNXI_CCM_BASE + PLL_VIDEO3_CTRL_REG)
#define CCMU_PLL_VE_CTRL_REG                    (SUNXI_CCM_BASE + PLL_VE_CTRL_REG)
#define CCMU_PLL_CPUA_CLK_REG                  (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x60)
#define CCMU_PLL_CPUB_CLK_REG                  (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x64)
#define CCMU_PLL_CPU_CLK_REG                  (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x68)
#define CCMU_PLL_DSU_CLK_REG                  (SUNXI_CPU_SYS_CFG_BASE + 0x817000 + 0x6C)
#define CCMU_PLL_AUDIO_CTRL_REG                 (SUNXI_CCM_BASE + PLL_AUDIO_CTRL_REG)


#define CCMU_PLL_HSIC_CTRL_REG            (SUNXI_CCM_BASE + 0x70)


/* cfg list */
//#define CCMU_CPUX_AXI_CFG_REG              (SUNXI_CCM_BASE + CPU_CLK_REG)
#define CCMU_AHB0_CFG_REG                  (SUNXI_CCM_BASE + 0x510)
#define CCMU_APB0_CFG_REG                  (SUNXI_CCM_BASE + 0x520)
#define CCMU_APB1_CFG_REG                  (SUNXI_CCM_BASE + 0x524)
#define CCMU_MBUS_CFG_REG                  (SUNXI_CCM_BASE + 0x540)

#define CCMU_CE_CLK_REG                    (SUNXI_CCM_BASE + 0x680)
#define CCMU_CE_BGR_REG                    (SUNXI_CCM_BASE + 0x68C)

#define CCMU_VE_CLK_REG                    (SUNXI_CCM_BASE + 0x690)
#define CCMU_VE_BGR_REG                    (SUNXI_CCM_BASE + 0x69C)

/*SYS*/
#define CCMU_DMA_BGR_REG                   (SUNXI_CCM_BASE + 0x70C)
//#define CCMU_AVS_CLK_REG                   (SUNXI_CCM_BASE + 0x750)
//#define CCMU_AVS_BGR_REG                   (SUNXI_CCM_BASE + 0x74C)

/*IOMMU*/
#define CCMU_IOMMU_BGR_REG                 (SUNXI_CCM_BASE + 0x7bc)
#define IOMMU_AUTO_GATING_REG              (SUNXI_IOMMU_BASE + 0X40)


/* storage */
#define CCMU_DRAM_CLK_REG                  (SUNXI_CCM_BASE + 0x800)
#define CCMU_MBUS_MST_CLK_GATING_REG       (SUNXI_CCM_BASE + 0x804)
//#define CCMU_PLL_DDR_AUX_REG               (SUNXI_CCM_BASE + 0x808)
#define CCMU_DRAM_BGR_REG                  (SUNXI_CCM_BASE + 0x80C)

#define CCMU_NAND_CLK_REG                  (SUNXI_CCM_BASE + 0x810)
#define CCMU_NAND_BGR_REG                  (SUNXI_CCM_BASE + 0x82C)

#define CCMU_SDMMC0_CLK_REG                (SUNXI_CCM_BASE + 0x830)
#define CCMU_SDMMC1_CLK_REG                (SUNXI_CCM_BASE + 0x834)
#define CCMU_SDMMC2_CLK_REG                (SUNXI_CCM_BASE + 0x838)
#define CCMU_SMHC_BGR_REG                  (SUNXI_CCM_BASE + 0x84c)

/*normal interface*/
#define CCMU_UART_BGR_REG                  (SUNXI_CCM_BASE + 0x90C)
#define CCMU_TWI_BGR_REG                   (SUNXI_CCM_BASE + 0x91C)

//#define CCMU_SCR_BGR_REG                   (SUNXI_CCM_BASE + 0x93C)

#define CCMU_SPI0_CLK_REG                  (SUNXI_CCM_BASE + 0x940)
#define CCMU_SPI1_CLK_REG                  (SUNXI_CCM_BASE + 0x944)
#define CCMU_SPI_BGR_CLK_REG               (SUNXI_CCM_BASE + 0x96C)
#define CCMU_USB0_CLK_REG                  (SUNXI_CCM_BASE + 0xA70)
#define CCMU_USB_BGR_REG                   (SUNXI_CCM_BASE + 0xA8C)

/*DMA*/
#define DMA_GATING_BASE                  CCMU_DMA_BGR_REG
#define DMA_GATING_PASS                  (1)
#define DMA_GATING_BIT                   (0)

/*CE*/
#define CE_CLK_SRC_MASK                  (0x7)
#define CE_CLK_SRC_SEL_BIT               (CE_CLK_REG_CLK_SRC_SEL_OFFSET)
#define CE_CLK_SRC                       (CE_CLK_REG_CLK_SRC_SEL_PERI0_400M)

#define CE_CLK_DIV_RATION_N_BIT          (0)
#define CE_CLK_DIV_RATION_N_MASK         (0x0)
#define CE_CLK_DIV_RATION_N              (0)

#define CE_CLK_DIV_RATION_M_BIT          (CE_CLK_REG_FACTOR_M_OFFSET)
#define CE_CLK_DIV_RATION_M_MASK         (CE_CLK_REG_FACTOR_M_CLEAR_MASK)
#define CE_CLK_DIV_RATION_M              (0)

#define CE_SCLK_ONOFF_BIT                (31)
#define CE_SCLK_ON                       (1)

#define CE_GATING_BASE                   CCMU_CE_BGR_REG
#define CE_GATING_PASS                   (1)
#define CE_GATING_BIT                    (0)

#define CE_RST_REG_BASE                  CCMU_CE_BGR_REG

#define CE_SYS_RST_BIT                    (CE_BGR_REG_CE_SYS_RST_OFFSET)
#define CE_RST_BIT                        (CE_BGR_REG_CE_RST_OFFSET)
#define CE_DEASSERT                       (CE_BGR_REG_CE_SYS_RST_DE_ASSERT)
#define CE_SYS_GATING_BIT                 (CE_BGR_REG_CE_SYS_GATING_OFFSET)

/*gpadc gate and reset reg*/
#define CCMU_GPADC_BGR_REG            (SUNXI_CCM_BASE + 0x09EC)
/*gpadc gate and reset reg*/
#define CCMU_GPADC_CLK_REG            (SUNXI_CCM_BASE + 0x09E0)
/*lpadc gate and reset reg*/
#define CCMU_LRADC_BGR_REG            (SUNXI_CCM_BASE + 0x0A9C)

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

/* SPIF clock bit field */
#define CCM_SPIF_CTRL_M(x)		((x) - 1)
#define CCM_SPIF_CTRL_N(x)		((x) << 8)
#define CCM_SPIF_CTRL_HOSC		(0x0 << 24)
#define CCM_SPIF_CTRL_PERI400M		(0x1 << 24)
#define CCM_SPIF_CTRL_PERI300M		(0x2 << 24)
#define CCM_SPIF_CTRL_ENABLE		(0x1 << 31)
#define GET_SPIF_CLK_SOURECS(x)		(x == CCM_SPIF_CTRL_PERI400M ? 400000000 : 300000000)
#define CCM_SPIF_CTRL_PERI		CCM_SPIF_CTRL_PERI400M
#define SPIF_RESET_SHIFT		(19)
#define SPIF_GATING_SHIFT		(3)

