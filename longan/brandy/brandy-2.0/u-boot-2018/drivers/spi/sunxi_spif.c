/*
 * (C) Copyright 2022-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * lujianliang <lujianliang@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <memalign.h>
#include <spi.h>
#include <linux/errno.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/gpio.h>
#include <sunxi_board.h>
#include "spif-sunxi.h"
#include <sys_config.h>
#include <fdt_support.h>
#include <linux/mtd/spi-nor.h>
#include <private_boot0.h>
#include <private_toc.h>
#include "../mtd/spi/sf_internal.h"
#include <linux/sizes.h>
#include <boot_param.h>

#define	SUNXI_SPI_MAX_TIMEOUT	1000000
#define	SUNXI_SPI_PORT_OFFSET	0x1000
#define SUNXI_SPI_DEFAULT_CLK  (40000000)

/* For debug */
#define SPIF_DEBUG 0

#if SPIF_DEBUG
#define SPIF_EXIT()		printf("%s()%d - %s\n", __func__, __LINE__, "Exit")
#define SPIF_ENTER()		printf("%s()%d - %s\n", __func__, __LINE__, "Enter ...")
#define SPIF_DBG(fmt, arg...)	printf("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SPIF_INF(fmt, arg...)	printf("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SPIF_ERR(fmt, arg...)	printf("%s()%d - "fmt, __func__, __LINE__, ##arg)

#else
#define SPIF_EXIT()		pr_debug("%s()%d - %s\n", __func__, __LINE__, "Exit")
#define SPIF_ENTER()		pr_debug("%s()%d - %s\n", __func__, __LINE__, "Enter ...")
#define SPIF_DBG(fmt, arg...)	pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SPIF_INF(fmt, arg...)	pr_debug("%s()%d - "fmt, __func__, __LINE__, ##arg)
#define SPIF_ERR(fmt, arg...)	printf("%s()%d - "fmt, __func__, __LINE__, ##arg)
#endif
#define SUNXI_SPIF_OK   0
#define SUNXI_SPIF_FAIL -1

struct sunxi_spif_slave *g_sspif;
static int double_clk_flag;

struct sunxi_spif_slave *get_sspif(void)
{
	return g_sspif;
}

static inline struct sunxi_spif_slave *to_sunxi_slave(struct spi_slave *slave)
{
	return container_of(slave, struct sunxi_spif_slave, slave);
}

#if SPIF_DEBUG
int spif_debug_flag;
static void spif_print_info(struct sunxi_spif_slave *sspif)
{
	char buf[1024] = {0};
	snprintf(buf, sizeof(buf)-1,
			"sspif->base_addr = 0x%x, the SPIF control register:\n"
			"[VER] 0x%02x = 0x%08x, [GC]  0x%02x = 0x%08x, [GCA] 0x%02x = 0x%08x\n"
			"[TCR] 0x%02x = 0x%08x, [TDS] 0x%02x = 0x%08x, [INT] 0x%02x = 0x%08x\n"
			"[STA] 0x%02x = 0x%08x, [CSD] 0x%02x = 0x%08x, [PHC] 0x%02x = 0x%08x\n"
			"[TCF] 0x%02x = 0x%08x, [TCS] 0x%02x = 0x%08x, [TNM] 0x%02x = 0x%08x\n"
			"[PSR] 0x%02x = 0x%08x, [PSA] 0x%02x = 0x%08x, [PEA] 0x%02x = 0x%08x\n"
			"[PMA] 0x%02x = 0x%08x, [DMA] 0x%02x = 0x%08x, [DSC] 0x%02x = 0x%08x\n"
			"[DFT] 0x%02x = 0x%08x, [CFT] 0x%02x = 0x%08x, [CFS] 0x%02x = 0x%08x\n"
			"[BAT] 0x%02x = 0x%08x, [BAC] 0x%02x = 0x%08x, [TB]  0x%02x = 0x%08x\n"
			"[RB]  0x%02x = 0x%08x\n",
			sspif->base_addr,
			SPIF_VER_REG, readl(sspif->base_addr + SPIF_VER_REG),
			SPIF_GC_REG, readl(sspif->base_addr + SPIF_GC_REG),
			SPIF_GCA_REG, readl(sspif->base_addr + SPIF_GCA_REG),

			SPIF_TC_REG, readl(sspif->base_addr + SPIF_TC_REG),
			SPIF_TDS_REG, readl(sspif->base_addr + SPIF_TDS_REG),
			SPIF_INT_EN_REG, readl(sspif->base_addr + SPIF_INT_EN_REG),

			SPIF_INT_STA_REG, readl(sspif->base_addr + SPIF_INT_STA_REG),
			SPIF_CSD_REG, readl(sspif->base_addr + SPIF_CSD_REG),
			SPIF_PHC_REG, readl(sspif->base_addr + SPIF_PHC_REG),

			SPIF_TCF_REG, readl(sspif->base_addr + SPIF_TCF_REG),
			SPIF_TCS_REG, readl(sspif->base_addr + SPIF_TCS_REG),
			SPIF_TNM_REG, readl(sspif->base_addr + SPIF_TNM_REG),

			SPIF_PS_REG, readl(sspif->base_addr + SPIF_PS_REG),
			SPIF_PSA_REG, readl(sspif->base_addr + SPIF_PSA_REG),
			SPIF_PEA_REG, readl(sspif->base_addr + SPIF_PEA_REG),

			SPIF_PMA_REG, readl(sspif->base_addr + SPIF_PMA_REG),
			SPIF_DMA_CTL_REG, readl(sspif->base_addr + SPIF_DMA_CTL_REG),
			SPIF_DSC_REG, readl(sspif->base_addr + SPIF_DSC_REG),

			SPIF_DFT_REG, readl(sspif->base_addr + SPIF_DFT_REG),
			SPIF_CFT_REG, readl(sspif->base_addr + SPIF_CFT_REG),
			SPIF_CFS_REG, readl(sspif->base_addr + SPIF_CFS_REG),

			SPIF_BAT_REG, readl(sspif->base_addr + SPIF_BAT_REG),
			SPIF_BAC_REG, readl(sspif->base_addr + SPIF_BAC_REG),
			SPIF_TB_REG, readl(sspif->base_addr + SPIF_TB_REG),

			SPIF_RB_REG, readl(sspif->base_addr + SPIF_RB_REG));
			printf("%s\n\n", buf);
}

void spif_print_descriptor(struct spif_descriptor_op *spif_op)
{
	char buf[512] = {0};
	snprintf(buf, sizeof(buf)-1,
			"hburst_rw_flag        : 0x%x\n"
			"block_data_len        : 0x%x\n"
			"data_addr             : 0x%x\n"
			"next_des_addr         : 0x%x\n"
			"trans_phase	       : 0x%x\n"
			"flash_addr	       : 0x%x\n"
			"cmd_mode_buswidth     : 0x%x\n"
			"addr_dummy_data_count : 0x%x\n",
			spif_op->hburst_rw_flag,
			spif_op->block_data_len,
			spif_op->data_addr,
			spif_op->next_des_addr,
			spif_op->trans_phase,
			spif_op->flash_addr,
			spif_op->cmd_mode_buswidth,
			spif_op->addr_dummy_data_count);
			printf("%s\n", buf);
	printf("spif_op addr [%x]...\n", (u32)spif_op);
}

#endif

static s32 sunxi_spif_gpio_request(void)
{
	int ret = 0;

	ret = fdt_set_all_pin("spif", "pinctrl-0");
	if (ret < 0) {
		SPIF_INF("set pin of spi0 fail %d\n", ret);
		return -1;
	}

	return 0;
}

static s32 sunxi_get_spif_mode(void)
{
	int nodeoffset = 0;
	int ret = 0;
	u32 rval = 0;
	u32 mode = 0;

	nodeoffset =  fdt_path_offset(working_fdt, "spif/spif-nor");
	if (nodeoffset < 0) {
		SPIF_INF("get spif para fail\n");
		return -1;
	}

	ret = fdt_getprop_u32(working_fdt, nodeoffset, "spif-rx-bus-width",
			(uint32_t *)(&rval));
	if (ret < 0) {
		SPIF_INF("get spif-rx-bus-width fail %d\n", ret);
		return -2;
	}

	if (rval == 2) {
		mode |= SPI_RX_DUAL;
	} else if (rval == 4) {
		mode |= SPI_RX_QUAD;
	} else if (rval == 8) {
		mode |= SPI_RX_OCTAL;
	} else
		mode |= SPI_RX_SLOW;

	ret = fdt_getprop_u32(working_fdt, nodeoffset, "spif-tx-bus-width",
			(uint32_t *)(&rval));
	if (ret < 0) {
		SPIF_INF("get spif-tx-bus-width fail %d\n", ret);
		return -3;
	}
	if (rval == 2) {
		mode |= SPI_TX_DUAL;
	} else if (rval == 4) {
		mode |= SPI_TX_QUAD;
	} else if (rval == 8) {
		mode |= SPI_TX_OCTAL;
	} else
		mode |= SPI_TX_BYTE;

	return mode;
}

static void spif_big_little_endian(bool endian, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	if (endian == LSB_FIRST)
		reg_val |= (SPIF_GC_RX_CFG_FBS | SPIF_GC_TX_CFG_FBS);
	else
		reg_val &= ~(SPIF_GC_RX_CFG_FBS | SPIF_GC_TX_CFG_FBS);
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_clean_mode_en(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	reg_val &= ~(SPIF_GC_NMODE_EN | SPIF_GC_PMODE_EN);
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_wp_en(bool enable, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	if (enable)
		reg_val |= SPIF_GC_WP_EN;
	else
		reg_val &= ~SPIF_GC_WP_EN;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_hold_en(bool enable, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	if (enable)
		reg_val |= SPIF_GC_HOLD_EN;
	else
		reg_val &= ~SPIF_GC_HOLD_EN;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_set_cs_pol(bool pol, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	if (pol)
		reg_val |= SPIF_GC_CS_POL;
	else
		reg_val &= ~SPIF_GC_CS_POL;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

/* spif config chip select */
static s32 spif_set_cs(u32 chipselect, void __iomem *base_addr)
{
	int ret;
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	if (chipselect < 4) {
		reg_val &= ~SPIF_GC_SS_MASK;/* SS-chip select, clear two bits */
		reg_val |= chipselect << SPIF_GC_SS_BIT_POS;/* set chip select */
		reg_val |= SPIF_GC_CS_POL;/* active low polarity */
		writel(reg_val, base_addr + SPIF_GC_REG);
		ret = SUNXI_SPIF_OK;
	} else {
		SPIF_ERR("Chip Select set fail! cs = %d\n", chipselect);
		ret = SUNXI_SPIF_FAIL;
	}

	return ret;
}

static void spif_set_mode(u32 spi_mode, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	reg_val &= ~SPIF_MASK;
	reg_val |= spi_mode;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

void spif_samp_dl_sw_rx_status(void __iomem  *base_addr, unsigned int status)
{
	unsigned int rval = readl(base_addr + SPIF_TC_REG);

	if (status)
		rval |= SPIF_ANALOG_DL_SW_RX_EN;
	else
		rval &= ~SPIF_ANALOG_DL_SW_RX_EN;

	writel(rval, base_addr +SPIF_TC_REG);
}

static void spif_samp_mode(void __iomem  *base_addr, unsigned int status)
{
	unsigned int rval = readl(base_addr + SPIF_TC_REG);

	if (status)
		rval |= SPIF_DIGITAL_ANALOG_EN;
	else
		rval &= ~SPIF_DIGITAL_ANALOG_EN;

	writel(rval, base_addr + SPIF_TC_REG);
}

static void spif_set_sample_mode(void __iomem *base_addr, unsigned int mode)
{
	unsigned int rval = readl(base_addr + SPIF_TC_REG);

	rval &= (~SPIF_DIGITAL_DELAY_MASK);
	rval |= mode << SPIF_DIGITAL_DELAY;
	writel(rval, base_addr + SPIF_TC_REG);
}

static void spif_set_sample_delay(void __iomem  *base_addr,
		unsigned int sample_delay)
{
	unsigned int rval = readl(base_addr + SPIF_TC_REG);

	rval &= (~SPIF_ANALOG_DELAY_MASK);
	rval |= sample_delay << SPIF_ANALOG_DELAY;
	writel(rval, base_addr + SPIF_TC_REG);
	mdelay(1);
}

static void spif_config_tc(struct sunxi_spif_slave *sspif)
{
	void __iomem *base_addr = (void *)(ulong)sspif->base_addr;

	if (sspif->right_sample_mode != SAMP_MODE_DL_DEFAULT) {
		spif_samp_mode(base_addr, 1);
		spif_samp_dl_sw_rx_status(base_addr, 1);
		spif_set_sample_mode(base_addr, sspif->right_sample_mode);
		spif_set_sample_delay(base_addr, sspif->right_sample_delay);
	}
}

static void spif_set_dqs(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_DFT_REG);

	reg_val |= SPIF_DFT_DQS;
	writel(reg_val, base_addr + SPIF_DFT_REG);
}

static void spif_set_cdc(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_CFT_REG);

	reg_val = SPIF_CFT_CDC;
	writel(reg_val, base_addr + SPIF_CFT_REG);
}

static void spif_set_csd(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_CSD_REG);

	reg_val |= SPIF_CSD_DEF;
	writel(reg_val, base_addr + SPIF_CSD_REG);
}

/* soft reset spif controller */
static void spif_soft_reset_fifo(void __iomem *base_addr)
{
	unsigned int timeout = 0xfffff;
	u32 reg_val = readl(base_addr + SPIF_GCA_REG);

	reg_val |= SPIF_GCA_SRST;
	writel(reg_val, base_addr + SPIF_GCA_REG);

	while (readl(base_addr + SPIF_GCA_REG) & SPIF_GCA_SRST) {
		timeout--;
		if (!timeout) {
			printf("SPIF soft reset time_out\n");
			return;
		}
	}
}

static void spif_reset_fifo(void __iomem *base_addr)
{
	unsigned int timeout = 0xfffff;
	u32 reg_val = readl(base_addr + SPIF_GCA_REG);

	reg_val |= SPIF_FIFO_SRST;

	writel(reg_val, base_addr + SPIF_GCA_REG);

	while (readl(base_addr + SPIF_GCA_REG) & SPIF_FIFO_SRST) {
		timeout--;
		if (!timeout) {
			printf("SPIF fifo reset time_out\n");
			return;
		}
	}
}

static void spif_set_trans_mode(u8 mode, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	if (mode)
		reg_val |= SPIF_GC_CFG_MODE;
	else
		reg_val &= ~SPIF_GC_CFG_MODE;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

/* set first descriptor start addr */
static void spif_set_des_start_addr(struct spif_descriptor_op *spif_op,
				void __iomem *base_addr)
{
	flush_cache((ulong)spif_op,
		    ALIGN(sizeof(struct spif_descriptor_op),
			    CONFIG_SYS_CACHELINE_SIZE));
	writel((u32)spif_op, base_addr + SPIF_DSC_REG);
}

/* set descriptor len */
static void spif_set_des_len(int len, void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_DMA_CTL_REG);

	reg_val |= len;
	writel(reg_val, base_addr + SPIF_DMA_CTL_REG);
}

/* DMA start Signal */
static void spif_dma_start_signal(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_DMA_CTL_REG);

	reg_val |= CFG_DMA_START;
	writel(reg_val, base_addr + SPIF_DMA_CTL_REG);
}

static void spif_trans_type_enable(u32 type_phase, void __iomem *base_addr)
{
	writel(type_phase, base_addr + SPIF_PHC_REG);
}

static void spif_set_flash_addr(u32 flash_addr, void __iomem *base_addr)
{
	writel(flash_addr, base_addr + SPIF_TCF_REG);
}

static void spif_set_buswidth(u32 cmd_mode_buswidth, void __iomem *base_addr)
{
	writel(cmd_mode_buswidth, base_addr + SPIF_TCS_REG);
}

static void spif_set_data_count(u32 addr_dummy_data_count, void __iomem *base_addr)
{
	writel(addr_dummy_data_count, base_addr + SPIF_TNM_REG);
}

static void spif_cpu_start_transfer(void __iomem *base_addr)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	reg_val |= SPIF_GC_NMODE_EN;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

static void spif_set_output_clk(void __iomem *base_addr, u32 status)
{
	u32 reg_val = readl(base_addr + SPIF_TC_REG);

	if (status)
		reg_val |= SPIF_CLK_SCKOUT_SRC_SEL;
	else
		reg_val &= ~SPIF_CLK_SCKOUT_SRC_SEL;
	writel(reg_val, base_addr + SPIF_TC_REG);
}

static void spif_set_dtr(void __iomem *base_addr, u32 status)
{
	u32 reg_val = readl(base_addr + SPIF_GC_REG);

	if (status)
		reg_val |= SPIF_GC_DTR_EN;
	else
		reg_val &= ~SPIF_GC_DTR_EN;
	writel(reg_val, base_addr + SPIF_GC_REG);
}

/* check the valid of cs id */
static int sunxi_spif_check_cs(unsigned int bus, unsigned int cs)
{
	int ret = SUNXI_SPIF_FAIL;

	if ((bus == 0) && (cs == 0))
		return SUNXI_SPIF_OK;

	return ret;
}

static int sunxi_spif_clk_init(u32 bus, u32 mod_clk)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned long mclk_base =
		(unsigned long)&ccm->spif_clk_cfg + bus * 0x4;
	u32 source_clk = 0;
	u32 rval = 0;
	u32 m, n, div;

	/* SCLK = src/M/N */
	/* N: 00:1 01:2 10:4 11:8 */
#ifdef FPGA_PLATFORM
	n = 0;
	m = 1;
	rval = CCM_SPIF_CTRL_ENABLE | CCM_SPIF_CTRL_N(n) | CCM_SPIF_CTRL_M(m);;
	source_clk = 24000000;
#else

	source_clk = GET_SPIF_CLK_SOURECS(CCM_SPIF_CTRL_PERI);
	SPIF_INF("source_clk: %d Hz, mod_clk: %d Hz\n", source_clk, mod_clk);

	div = (source_clk + mod_clk - 1) / mod_clk;
	div = div == 0 ? 1 : div;
	if (div > 128) {
		m = 1;
		n = 0;
		return -1;
	} else if (div > 64) {
		n = 3;
		m = div >> 3;
	} else if (div > 32) {
		n = 2;
		m = div >> 2;
	} else if (div > 16) {
		n = 1;
		m = div >> 1;
	} else {
		n = 0;
		m = div;
	}

	rval = CCM_SPIF_CTRL_ENABLE | CCM_SPIF_CTRL_PERI |
			CCM_SPIF_CTRL_N(n) | CCM_SPIF_CTRL_M(m);

#endif
	writel(rval, (volatile void __iomem *)mclk_base);

	/* spif reset */
	setbits_le32(&ccm->spi_gate_reset, (0<<SPIF_RESET_SHIFT));
	setbits_le32(&ccm->spi_gate_reset, (1<<SPIF_RESET_SHIFT));

	/* spif gating */
	setbits_le32(&ccm->spi_gate_reset, (1<<SPIF_GATING_SHIFT));
	/*sclk_freq = source_clk / (1 << n) / m*/

	SPIF_INF("src: %d Hz, spic:%d Hz,  n=%d, m=%d\n",
			source_clk, source_clk/ (1 << n) / m, n, m);

	return 0;
}

/*
static int sunxi_get_spif_clk(int bus)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;
	unsigned long mclk_base =
		(unsigned long)&ccm->spif_clk_cfg + bus * 0x4;
	u32 reg_val = 0;
	u32 src = 0, clk = 0, sclk_freq = 0;
	u32 n, m;

	reg_val = readl((volatile void __iomem *)mclk_base);
	src = (reg_val >> 24) & 0x7;
	n = (reg_val >> 8) & 0x3;
	m = ((reg_val >> 0) & 0xf) + 1;

	switch(src) {
		case 0:
			clk = 24000000;
			break;
		case 1:
			clk = GET_SPIF_CLK_SOURECS(CCM_SPIF_CTRL_PERI400M);
			break;
		case 2:
			clk = GET_SPIF_CLK_SOURECS(CCM_SPIF_CTRL_PERI300M);
			break;
		default:
			clk = 0;
			break;
	}
	sclk_freq = clk / (1 << n) / m;
	SPIF_INF("sclk_freq= %d Hz,reg_val: %x , n=%d, m=%d\n",
			sclk_freq, reg_val, n, m);
	return sclk_freq;

}
*/

static int sunxi_spif_clk_exit(void)
{
	struct sunxi_ccm_reg *const ccm =
		(struct sunxi_ccm_reg *)SUNXI_CCM_BASE;

	/* spif gating */
	clrbits_le32(&ccm->spi_gate_reset, 1<<SPIF_GATING_SHIFT);

	/* spif reset */
	clrbits_le32(&ccm->spi_gate_reset, 1<<SPIF_RESET_SHIFT);
	return 0;
}

void spif_init_clk(struct spi_slave *slave)
{
	struct sunxi_spif_slave *sunxi_slave = to_sunxi_slave(slave);
	int ret = 0, nodeoffset = 0;
	u32 rval = 0;

	nodeoffset =  fdt_path_offset(working_fdt, "spif/spif-nor");
	if (nodeoffset < 0) {
		SPIF_ERR("get spi0 para fail\n");
	} else {
		ret = fdt_getprop_u32(working_fdt, nodeoffset,
				"spif-max-frequency", (uint32_t *)(&rval));
		if (ret < 0) {
			SPIF_ERR("get spif-max-frequency fail %d\n", ret);
		} else
			sunxi_slave->max_hz = rval;
	}

	pr_force("spif sunxi_slave->max_hz:%d \n", sunxi_slave->max_hz);
	/* clock */
	if (sunxi_spif_clk_init(0, sunxi_slave->max_hz))
		SPIF_ERR("spi clk init error\n");

	double_clk_flag = 0;
	return;
}

static void spif_dtr_enable(struct sunxi_spif_slave *sspif,
		struct spif_descriptor_op *spif_op)
{
	void __iomem *base_addr = (void *)(ulong)sspif->base_addr;
	unsigned int bus = sspif->slave.bus;
	unsigned int clk = sspif->max_hz;
	unsigned int dtr_double_clk = clk * 2;

	if (!sspif->rx_dtr_en && !sspif->tx_dtr_en)
		return;

	if ((spif_op->cmd_mode_buswidth >> SPIF_ADDR_TRANS_POS) & 0x3) {
		if ((spif_op->trans_phase & SPIF_RX_TRANS_EN) &&
				sspif->rx_dtr_en) {
			spif_set_output_clk(base_addr, 1);
			spif_set_dtr(base_addr, 1);
			if (!double_clk_flag) {
				sunxi_spif_clk_init(bus, dtr_double_clk);
				double_clk_flag = 1;
			}
		} else if (spif_op->trans_phase & SPIF_TX_TRANS_EN &&
				sspif->tx_dtr_en) {
			spif_set_output_clk(base_addr, 1);
			spif_set_dtr(base_addr, 1);
			if (!dtr_double_clk) {
				sunxi_spif_clk_init(bus, dtr_double_clk);
				double_clk_flag = 1;
			}
		}
	} else {
		spif_set_output_clk(base_addr, 0);
		spif_set_dtr(base_addr, 0);
		if (double_clk_flag) {
			sunxi_spif_clk_init(bus, clk);
			double_clk_flag = 0;
		}
	}
}

void spif_init_dtr(u32 flags)
{
	struct sunxi_spif_slave *sspif = get_sspif();

	if (flags & USE_RX_DTR)
		sspif->rx_dtr_en = 1;
	else
		sspif->rx_dtr_en = 0;

	if (flags & USE_TX_DTR)
		sspif->tx_dtr_en = 1;
	else
		sspif->tx_dtr_en = 0;
}

int spif_cs_is_valid(unsigned int bus, unsigned int cs)
{
	if (sunxi_spif_check_cs(bus, cs) == SUNXI_SPIF_OK)
		return 1;
	else
		return 0;
}

struct spi_slave *spif_setup_slave(unsigned int bus, unsigned int cs,
				  unsigned int max_hz, unsigned int mode)
{
	struct sunxi_spif_slave *sunxi_slave;
	int ret = 0;

	SPIF_ENTER();
	if (!spif_cs_is_valid(bus, cs)) {
		printf("sunxi_spi: invalid bus %d / chip select %d\n", bus, cs);
		return NULL;
	}

	sunxi_slave = spi_alloc_slave(struct sunxi_spif_slave, bus, cs);
	if (!sunxi_slave)
		return NULL;

	sunxi_slave->max_hz = max_hz;
	sunxi_slave->mode = mode;
	sunxi_slave->base_addr = SUNXI_SPIF_BASE + bus * SUNXI_SPI_PORT_OFFSET;
	sunxi_slave->right_sample_delay = SAMP_MODE_DL_DEFAULT;
	sunxi_slave->right_sample_mode = SAMP_MODE_DL_DEFAULT;
	sunxi_slave->rx_dtr_en = 0;
	sunxi_slave->tx_dtr_en = 0;

	sunxi_slave->slave.mode = mode;
	g_sspif = sunxi_slave;

	/* gpio */
	sunxi_spif_gpio_request();

	/*rx/tx bus width*/
	ret = sunxi_get_spif_mode();
	if (ret > 0) {
		sunxi_slave->mode = ret;
		sunxi_slave->slave.mode = ret;
	}

	/* clock */
	if (sunxi_spif_clk_init(0, max_hz)) {
		free(sunxi_slave);
		return NULL;
	}

	return &sunxi_slave->slave;

}

void spif_free_slave(struct spi_slave *slave)
{
	struct sunxi_spif_slave *sunxi_slave = to_sunxi_slave(slave);
	SPIF_ENTER();
	free(sunxi_slave);

	/* disable module clock */
	sunxi_spif_clk_exit();
}

int spif_claim_bus(struct spi_slave *slave)
{
	struct sunxi_spif_slave *sspif = to_sunxi_slave(slave);
	void __iomem *base_addr =
		(void __iomem *)(unsigned long)sspif->base_addr;

	SPIF_ENTER();

	/* 1. reset all tie logic & fifo */
	spif_soft_reset_fifo(base_addr);
	spif_clean_mode_en(base_addr);

	/* 2. interface first transmit bit select */
	spif_big_little_endian(MSB_FIRST, base_addr);

	/* 3. disable wp & hold */
	spif_wp_en(0, base_addr);
	spif_hold_en(0, base_addr);

	/* 4. disable DTR */
	spif_set_output_clk(base_addr, 0);
	spif_set_dtr(base_addr, 0);

	/* 5. set the default chip select */
	spif_set_cs(slave->cs, base_addr);
	spif_set_cs_pol(1, base_addr);

	/* 6. set spi CPOL and CPHA */
	spif_set_mode(SPIF_MODE0, base_addr);

	/* 7. set sample delay timing */
	spif_config_tc(sspif);

	/* 8. set reg defauld count */
	spif_set_dqs(base_addr);
	spif_set_cdc(base_addr);
	spif_set_csd(base_addr);

	return 0;
}

static void spif_ctr_recover(struct spi_slave *slave)
{
	sunxi_spif_clk_exit();
	spif_init_clk(slave);
	spif_claim_bus(slave);
}

void spif_update_right_delay_para(struct mtd_info *mtd)
{
	struct spi_nor *nor = mtd->priv;
	struct spi_slave *slave = nor->spi;
	struct sunxi_spif_slave *sspif = to_sunxi_slave(slave);
	void __iomem *base_addr = (void *)(ulong)sspif->base_addr;
	unsigned int sample_delay;
	unsigned int start_ok = 0, end_ok = 0, len_ok = 0, mode_ok = 0;
	unsigned int start_backup = 0, end_backup = 0, len_backup = 0;
	unsigned int mode = 0, startry_mode = 0, endtry_mode = 2, block = 0;
	u8 erase_opcode = nor->erase_opcode;
	uint32_t erasesize = mtd->erasesize;
	if (mtd->size > SZ_16M)
		nor->erase_opcode = SPINOR_OP_BE_32K_4B;
	else
		nor->erase_opcode = SPINOR_OP_BE_32K;
	mtd->erasesize = 32 * 1024;

	size_t retlen;
	u_char *cache_source;
	u_char *cache_target;
	u_char *cache_boot0;
	int len = nor->page_size;

	struct erase_info instr;
	instr.addr = block * mtd->erasesize;
	instr.len = (endtry_mode - startry_mode + 1) * mtd->erasesize;

	cache_boot0 = malloc_align(instr.len, 64);
	mtd->_read(mtd, instr.addr, instr.len, &retlen, cache_boot0);
	mtd->_erase(mtd, &instr);

	cache_source = malloc_align(len, 64);
	cache_target = malloc_align(len, 64);
	memset(cache_source, 0xa5, len);

	/* re-initialize from device tree */
	spif_init_clk(slave);

	spif_samp_mode(base_addr, 1);
	spif_samp_dl_sw_rx_status(base_addr, 1);
	for (mode = startry_mode; mode <= endtry_mode; mode++) {
		sspif->right_sample_mode = mode;
		spif_set_sample_mode(base_addr, mode);
		for (sample_delay = 0; sample_delay < 64; sample_delay++) {
			spif_set_sample_delay(base_addr, sample_delay);
			mtd->_write(mtd, block * mtd->erasesize +
					sample_delay * nor->page_size,
					len, &retlen, cache_source);
		}

		for (sample_delay = 0; sample_delay < 64; sample_delay++) {
			sspif->right_sample_delay = sample_delay;
			spif_set_sample_delay(base_addr, sample_delay);
			memset(cache_target, 0, len);
			mtd->_read(mtd, block * mtd->erasesize +
					sample_delay * nor->page_size,
					len, &retlen, cache_target);

			if (strncmp((char *)cache_source, (char *)cache_target,
						len) == 0) {
				pr_debug("mode:%d delat:%d [OK]\n",
						mode, sample_delay);
				if (!len_backup) {
					start_backup = sample_delay;
					end_backup = sample_delay;
				} else
					end_backup = sample_delay;
				len_backup++;
			} else {
				pr_debug("mode:%d delay:%d [ERROR]\n",
						mode, sample_delay);
				if (!start_backup)
					continue;
				else {
					if (len_backup > len_ok) {
						len_ok = len_backup;
						start_ok = start_backup;
						end_ok = end_backup;
						mode_ok = mode;
					}

					len_backup = 0;
					start_backup = 0;
					end_backup = 0;
				}
			}
		}

		if (len_backup > len_ok) {
			len_ok = len_backup;
			start_ok = start_backup;
			end_ok = end_backup;
			mode_ok = mode;
		}
		len_backup = 0;
		start_backup = 0;
		end_backup = 0;

		block++;
	}

	if (!len_ok) {
		sspif->right_sample_delay = SAMP_MODE_DL_DEFAULT;
		sspif->right_sample_mode = SAMP_MODE_DL_DEFAULT;
		spif_samp_mode(base_addr, 0);
		spif_samp_dl_sw_rx_status(base_addr, 0);
		/* clock */
		if (sunxi_spif_clk_init(0, 24000000))
			pr_err("spi clk init error\n");

		pr_info("spif update delay param error\n");
	} else {
		sspif->right_sample_delay = (start_ok + end_ok) / 2;
		sspif->right_sample_mode = mode_ok;
		spif_set_sample_delay(base_addr, sspif->right_sample_delay);
		spif_set_sample_mode(base_addr, sspif->right_sample_mode);
	}
	pr_info("Sample mode:%d start:%d end:%d right_sample_delay:0x%x\n",
			mode_ok, start_ok, end_ok,
			sspif->right_sample_delay);

	mtd->_write(mtd, instr.addr, instr.len, &retlen, cache_boot0);

	nor->erase_opcode = erase_opcode;
	mtd->erasesize = erasesize;
	free_align(cache_source);
	free_align(cache_target);
	free_align(cache_boot0);
	return ;
}

static void spif_boot_try_delay_param(struct mtd_info *mtd,
				boot_spinor_info_t *boot_info)
{
	struct spi_nor *nor = mtd->priv;
	struct spi_slave *slave = nor->spi;
	struct sunxi_spif_slave *sspif = to_sunxi_slave(slave);
	void __iomem *base_addr = (void *)(ulong)sspif->base_addr;
	unsigned int sample_delay;
	unsigned int start_ok = 0, end_ok = 0, len_ok = 0, mode_ok = 0;
	unsigned int start_backup = 0, end_backup = 0, len_backup = 0;
	unsigned int mode = 0, startry_mode = 0, endtry_mode = 2;
	size_t retlen, len = 512;
	boot0_file_head_t *boot0_head;
	boot0_head = malloc_align(len, 64);

	/* re-initialize from device tree */
	spif_init_clk(slave);

	spif_samp_mode(base_addr, 1);
	spif_samp_dl_sw_rx_status(base_addr, 1);
	for (mode = startry_mode; mode <= endtry_mode; mode++) {
		sspif->right_sample_mode = mode;
		spif_set_sample_mode(base_addr, mode);
		for (sample_delay = 0; sample_delay < 64; sample_delay++) {
			sspif->right_sample_delay = sample_delay;
			spif_set_sample_delay(base_addr, sample_delay);
			memset(boot0_head, 0, len);
			mtd->_read(mtd, 0, len, &retlen, (u_char *)boot0_head);

			if (strncmp((char *)boot0_head->boot_head.magic,
				(char *)BOOT0_MAGIC,
				sizeof(boot0_head->boot_head.magic)) == 0) {
				pr_debug("mode:%d delat:%d [OK]\n",
						mode, sample_delay);
				if (!len_backup) {
					start_backup = sample_delay;
					end_backup = sample_delay;
				} else
					end_backup = sample_delay;
				len_backup++;
			} else {
				pr_debug("mode:%d delay:%d [ERROR]\n",
						mode, sample_delay);
				if (!start_backup)
					continue;
				else {
					if (len_backup > len_ok) {
						len_ok = len_backup;
						start_ok = start_backup;
						end_ok = end_backup;
						mode_ok = mode;
					}

					len_backup = 0;
					start_backup = 0;
					end_backup = 0;
				}
			}
		}
		if (len_backup > len_ok) {
			len_ok = len_backup;
			start_ok = start_backup;
			end_ok = end_backup;
			mode_ok = mode;
		}
		len_backup = 0;
		start_backup = 0;
		end_backup = 0;
	}

	if (!len_ok) {
		sspif->right_sample_delay = SAMP_MODE_DL_DEFAULT;
		sspif->right_sample_mode = SAMP_MODE_DL_DEFAULT;
		spif_samp_mode(base_addr, 0);
		spif_samp_dl_sw_rx_status(base_addr, 0);
		/* clock */
		if (sunxi_spif_clk_init(0, 24000000))
			pr_err("spi clk init error\n");

		pr_err("spif update delay param error\n");
	} else {
		sspif->right_sample_delay = (start_ok + end_ok) / 2;
		sspif->right_sample_mode = mode_ok;
		spif_set_sample_delay(base_addr, sspif->right_sample_delay);
		spif_set_sample_mode(base_addr, sspif->right_sample_mode);
	}
	pr_info("Sample mode:%d start:%d end:%d right_sample_delay:0x%x\n",
			mode_ok, start_ok, end_ok,
			sspif->right_sample_delay);

	boot_info->sample_delay = sspif->right_sample_delay;
	boot_info->sample_mode = sspif->right_sample_mode;
	free_align(boot0_head);
	return;
}

extern int update_boot_param(struct spi_nor *flash);
int spif_set_right_delay_para(struct mtd_info *mtd)
{
	struct spi_nor *nor = mtd->priv;
	struct spi_slave *slave = nor->spi;
	struct sunxi_spif_slave *sspif = to_sunxi_slave(slave);
	void __iomem *base_addr = (void *)(ulong)sspif->base_addr;
	boot_spinor_info_t *boot_info = NULL;
	size_t retlen;

	struct sunxi_boot_param_region *boot_param = NULL;
	boot_param = malloc_align(BOOT_PARAM_SIZE, 64);

	mtd->_read(mtd, (CONFIG_SPINOR_UBOOT_OFFSET << 9) - BOOT_PARAM_SIZE,
			BOOT_PARAM_SIZE, &retlen, (u_char *)boot_param);
	boot_info = (boot_spinor_info_t *)boot_param->spiflash_info;

	if (strncmp((const char *)boot_param->header.magic,
				(const char *)BOOT_PARAM_MAGIC,
				sizeof(boot_param->header.magic)) ||
		strncmp((const char *)boot_info->magic,
				(const char *)SPINOR_BOOT_PARAM_MAGIC,
				sizeof(boot_info->magic))) {
		printf("boot param magic error\n");
		spif_boot_try_delay_param(mtd, boot_info);
		if (update_boot_param(nor))
			printf("update boot param error\n");
	}

	if (boot_info->sample_delay == SAMP_MODE_DL_DEFAULT) {
		printf("boot smple delay error\n");
		spif_boot_try_delay_param(mtd, boot_info);
		if (update_boot_param(nor))
			printf("update boot param error\n");
	}

	pr_force("spi sample_mode:%x sample_delay:%x\n",
			boot_info->sample_mode, boot_info->sample_delay);

	spif_init_clk(slave);

	spif_samp_mode(base_addr, 1);
	spif_samp_dl_sw_rx_status(base_addr, 1);
	spif_set_sample_mode(base_addr, boot_info->sample_mode);
	spif_set_sample_delay(base_addr, boot_info->sample_delay);
	sspif->right_sample_delay = boot_info->sample_delay;
	sspif->right_sample_mode = boot_info->sample_mode;

	free_align(boot_param);
	return 0;
}

int spif_xfer(struct spi_slave *slave, struct spif_descriptor_op *spif_op)
{
	int timeout = 0xfffff;
	struct sunxi_spif_slave *sspif = to_sunxi_slave(slave);
	void __iomem *base_addr =
		(void __iomem *)(unsigned long)sspif->base_addr;
	int data_len = spif_op->block_data_len & DMA_DATA_LEN;

	spif_reset_fifo(base_addr);
	spif_dtr_enable(sspif, spif_op);
	if ((spif_op->block_data_len & DMA_DATA_LEN) == 0) {
		spif_set_trans_mode(SPIF_GC_CPU_MODE, base_addr);

		spif_trans_type_enable(spif_op->trans_phase, base_addr);

		spif_set_flash_addr(spif_op->flash_addr, base_addr);

		spif_set_buswidth(spif_op->cmd_mode_buswidth, base_addr);

		spif_set_data_count(spif_op->addr_dummy_data_count, base_addr);

		spif_cpu_start_transfer(base_addr);

		while ((readl(base_addr + SPIF_GC_REG) & SPIF_GC_NMODE_EN)) {
			timeout--;
			if (!timeout) {
				spif_ctr_recover(slave);
				printf("SPIF DMA transfer time_out\n");
				return -1;
			}
		}
#if SPIF_DEBUG
		if (spif_debug_flag)
			spif_print_info(sspif);
#endif
	} else {
#ifndef CONFIG_SPI_USE_DMA
		spif_set_trans_mode(SPIF_GC_CPU_MODE, base_addr);

		spif_trans_type_enable(spif_op->trans_phase, base_addr);

		spif_set_flash_addr(spif_op->flash_addr, base_addr);

		spif_set_buswidth(spif_op->cmd_mode_buswidth, base_addr);

		spif_set_data_count(spif_op->addr_dummy_data_count, base_addr);
#else
		spif_set_trans_mode(SPIF_GC_DMA_MODE, base_addr);

		spif_op->addr_dummy_data_count |= SPIF_DES_NORMAL_EN;
#endif
		/* flush data addr */
		flush_cache((u32)spif_op->data_addr,
				data_len < CONFIG_SYS_CACHELINE_SIZE ?
				CONFIG_SYS_CACHELINE_SIZE : data_len);
		spif_set_des_start_addr(spif_op, base_addr);

		spif_set_des_len(DMA_DESCRIPTOR_LEN, base_addr);

		spif_dma_start_signal(base_addr);

		/*
		 *  The SPIF move data through DMA, and DMA and CPU modes
		 *  differ only between actively configuring registers and
		 *  configuring registers through the DMA descriptor
		 */
#ifndef CONFIG_SPI_USE_DMA
		spif_cpu_start_transfer(base_addr);
#endif
#if SPIF_DEBUG
		if (spif_debug_flag) {
			spif_print_descriptor(spif_op);
			spif_print_info(sspif);
		}
#endif
		/* waiting DMA finish */
		while (!(readl(base_addr + SPIF_INT_STA_REG) &
				DMA_TRANS_DONE_INT)) {
			timeout--;
			if (!timeout) {
				spif_ctr_recover(slave);
				printf("SPIF DMA transfer time_out\n");
				return -1;
			}
		}
		invalidate_dcache_range((u32)spif_op->data_addr,
				(u32)spif_op->data_addr +
				(data_len < CONFIG_SYS_CACHELINE_SIZE ?
				CONFIG_SYS_CACHELINE_SIZE : data_len));
		writel(DMA_TRANS_DONE_INT, base_addr + SPIF_INT_STA_REG);
	}

	return 0;
}

