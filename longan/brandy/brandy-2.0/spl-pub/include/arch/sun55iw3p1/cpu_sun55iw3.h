#include <arch/sun55iw3p1/cpu_autogen.h>
#define SUNXI_PIO_BASE                (SUNXI_GPIO_BASE)
#define SUNXI_R_PIO_BASE              (SUNXI_R_GPIO_BASE)
#define SUNXI_CCM_BASE                (SUNXI_CCMU_BASE)
#define SUNXI_DMA_BASE                (SUNXI_DMAC_BASE)
#define SUNXI_CE_BASE                 (SUNXI_CE_NS_BASE)
#define SUNXI_SS_BASE                 (SUNXI_CE_BASE)
#define SUNXI_CPUXCFG_BASE            (SUNXI_CPU_SYS_CFG_BASE)
#define SUNXI_SYSCRL_BASE             (SUNXI_SYSCTRL_BASE)
#define SUNXI_KEYADC_BASE             (SUNXI_LRADC_BASE)
#define SUNXI_RPRCM_BASE              (SUNXI_R_PRCM_BASE)
#define SUNXI_RPWM_BASE               (SUNXI_R_PWM_BASE)
#define SUNXI_RPIO_BASE               (SUNXI_R_GPIO_BASE)

#define SUNXI_RTWI_BASE               (SUNXI_R_TWI0_BASE)
#define SUNXI_RTWI_BRG_REG 	          (SUNXI_RPRCM_BASE + 0x019c)
#define SUNXI_RTWI0_RST_BIT			  (16)
#define SUNXI_RTWI0_GATING_BIT		  (0)
#define SUNXI_RST_BIT				  (16)
#define SUNXI_GATING_BIT			  (0)
#define SUNXI_RTC_DATA_BASE           (SUNXI_RTC_BASE + 0x100)

#define SUNXI_SMC_BASE                (SUNXI_MEMC_SMC_BASE)
#define SUNXI_CPUS_CFG_BASE			  (0x07000400U)
#define SUNXI_RCPUCFG_BASE			  (SUNXI_CPUS_CFG_BASE)

#define SUNXI_SPIF_BASE		      (0x047F0000)

/* use for usb correct */
#define VDD_SYS_PWROFF_GATING_REG			(SUNXI_RPRCM_BASE + 0x250)
#define RES_CAL_CTRL_REG                    (SUNXI_RPRCM_BASE + 0X310)
#define VDD_ADDA_OFF_GATING					(9)
#define CAL_ANA_EN							(1)
#define CAL_EN								(0)

#define RVBARADDR0_L						(SUNXI_CPUXCFG_BASE + 0x40)
#define RVBARADDR0_H						(SUNXI_CPUXCFG_BASE + 0x44)

#define SRAM_CONTRL_REG0					(SUNXI_SYSCRL_BASE + 0x0)
#define SRAM_CONTRL_REG1					(SUNXI_SYSCRL_BASE + 0x4)

#define GPIO_BIAS_MAX_LEN (32)
#define GPIO_BIAS_MAIN_NAME "gpio_bias"
#define GPIO_POW_MODE_REG	(0x0380)
#define GPIO_POW_MS_CTL	(0x0384)
#define GPIO_POW_MODE_VAL_REG	(0x0388)
#define GPIO_3_3V_MODE 0
#define GPIO_1_8V_MODE 1

#define PIOC_REG_o_POW_MOD_SEL 0x380
#define PIOC_REG_o_POW_MS_CTL 0x384
#define PIOC_REG_o_POW_MS_VAL 0x388

#define PIOC_REG_POW_MOD_SEL (SUNXI_PIO_BASE + PIOC_REG_o_POW_MOD_SEL)
#define PIOC_REG_POW_MS_CTL (SUNXI_PIO_BASE + PIOC_REG_o_POW_MS_CTL)
#define PIOC_REG_POW_VAL (SUNXI_PIO_BASE + PIOC_REG_o_POW_MS_VAL)

#define PIOC_SEL_Px_3_3V_VOL 1
#define PIOC_SEL_Px_1_8V_VOL 0

#define PIOC_CTL_Px_ENABLE 0
#define PIOC_CTL_Px_DISABLE 1

#define PIOC_VAL_Px_3_3V_VOL 0
#define PIOC_VAL_Px_1_8V_VOL 1

#define PIOC_CTL_Px_DEFUALT PIOC_CTL_Px_ENABLE
#define PIOC_SEL_Px_DEFAULT PIOC_SEL_Px_1_8V_VOL

