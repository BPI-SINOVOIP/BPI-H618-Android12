/*
 * drivers/spi/spif-sunxi.h
 *
 * Copyright (C) 2021 - 2024 Reuuimlla Limited
 * Lu Jian Liang <lujianliang@reuuimllatech.com>
 *
 * SUNXI SPIF Register Definition
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * 2021.12.1 <lujianliang@allwinnertech.com>
 *    Adapt to support sun8iw21~future of Allwinner.
 */

#ifndef _SUNXI_SPIF_H_
#define _SUNXI_SPIF_H_

#include <linux/mtd/mtd.h>
/*
#define SPI_MODULE_NUM		(4)
#define SPI_FIFO_DEPTH		(128)
#define MAX_FIFU		64
#define BULK_DATA_BOUNDARY	64
#define SPI_MAX_FREQUENCY	100000000
*/
/* SPIF Global (Additional) Control Register Bit Fields & Masks,default value:0x0000_0080 */

/* SPIF Registers offsets from peripheral base address */
#define SPIF_VER_REG		(0x00)	/* version number register */
#define SPIF_GC_REG		(0x04)	/* global control register */
#define SPIF_GCA_REG		(0x08)	/* global additional control register */
#define SPIF_TC_REG		(0x0C)	/* timing control register */
#define SPIF_TDS_REG		(0x10)	/* timing delay state register */
#define SPIF_INT_EN_REG		(0x14)	/* interrupt enable register */
#define SPIF_INT_STA_REG	(0x18)	/* interrupt status register */
#define SPIF_CSD_REG		(0x1C)	/* chipselect delay register */
#define SPIF_PHC_REG		(0x20)	/* trans phase configure register */
#define SPIF_TCF_REG		(0x24)	/* trans configure1 register */
#define SPIF_TCS_REG		(0x28)	/* trans configure2 register */
#define SPIF_TNM_REG		(0x2C)	/* trans number register */
#define SPIF_PS_REG		(0x30)	/* prefetch state register */
#define SPIF_PSA_REG		(0x34)	/* prefetch start address register */
#define SPIF_PEA_REG		(0x38)	/* prefetch end address register */
#define SPIF_PMA_REG		(0x3C)	/* prefetch map address register */
#define SPIF_DMA_CTL_REG	(0x40)	/* DMA control register */
#define SPIF_DSC_REG		(0x44)	/* DMA descriptor start address register */
#define SPIF_DFT_REG		(0x48)	/* DQS FIFO trigger level register */
#define SPIF_CFT_REG		(0x4C)	/* CDC FIFO trigger level register */
#define SPIF_CFS_REG		(0x50)	/* CDC FIFO status register */
#define SPIF_BAT_REG		(0x54)	/* Bit-Aligned tansfer configure register */
#define SPIF_BAC_REG		(0x58)	/* Bit-Aligned clock configuration register */
#define SPIF_TB_REG		(0x5C)	/* TX Bit register */
#define SPIF_RB_REG		(0x60)	/* RX Bit register */
#define SPIF_RXDATA_REG		(0x200)	/* prefetch RX data register */

/* SPIF global control register bit fields & masks,default value:0x0000_0100 */
#define SPIF_GC_CFG_MODE	(0x1 << 0)
#define SPIF_GC_DMA_MODE	(1)
#define SPIF_GC_CPU_MODE	(0)
#define SPIF_GC_ADDR_MAP_MODE	(0x1 << 1)
#define SPIF_GC_NMODE_EN	(0x1 << 2)
#define SPIF_GC_PMODE_EN	(0x1 << 3)
#define SPIF_GC_CPHA		(0x1 << 4)
#define SPIF_GC_CPOL		(0x1 << 5)
#define SPIF_GC_SS_MASK		(0x3 << 6) /* SPIF chip select:00-SPI_SS0;01-SPI_SS1;10-SPI_SS2;11-SPI_SS3*/
#define SPIF_GC_CS_POL		(0x1 << 8)
#define SPIF_GC_DUMMY_BIT_POL	(0x1 << 9)
#define SPIF_GC_DQS_RX_EN	(0x1 << 10)
#define SPIF_GC_HOLD_POL	(0x1 << 12)
#define SPIF_GC_HOLD_EN		(0x1 << 13)
#define SPIF_GC_WP_POL		(0x1 << 14)
#define SPIF_GC_WP_EN		(0x1 << 15)
#define SPIF_GC_DTR_EN		(0x1 << 16)
#define SPIF_GC_RX_CFG_FBS	(0x1 << 17)
#define SPIF_GC_TX_CFG_FBS	(0x1 << 18)
#define SPIF_GC_SS_BIT_POS	(6)
#define MSB_FIRST		(0)
#define LSB_FIRST		(1)

//SPIF_GCR:SPIF_MODE
#define SPIF_MODE0		(0U << 4)
#define SPIF_MODE1		(SPIF_GC_CPHA)
#define SPIF_MODE2		(SPIF_GC_CPOL)
#define SPIF_MODE3		(SPIF_GC_CPOL | SPIF_GC_CPHA)
#define SPIF_MASK		(3U << 4)

#define SPIF_SCKT_DELAY_MODE	(1U << 21)
#define SPIF_DIGITAL_ANALOG_EN	(1U << 20)
#define SPIF_DIGITAL_DELAY	(16)
#define SPIF_DIGITAL_DELAY_MASK	(7U << 16)
#define SPIF_ANALOG_DL_SW_RX_EN	(1U << 6)
#define SPIF_ANALOG_DELAY	(0)
#define SPIF_ANALOG_DELAY_MASK	(0x3F << 0)

#define SPIF_GCA_SRST		(0xf << 0)
#define SPIF_FIFO_SRST		(0x3 << 0)
#define SPIF_DFT_DQS		(0x6400)
#define SPIF_CFT_CDC		(0x64106410)
#define SPIF_CSDA		(5)
#define SPIF_CSEOT		(6)
#define	SPIF_CSSOT		(6)
#define SPIF_CSD_DEF		((SPIF_CSDA << 16) | (SPIF_CSEOT << 8) | SPIF_CSSOT)

/* SPIF Timing Configure Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_SAMP_DL_VAL_TX_POS (8)
#define SPIF_SAMP_DL_VAL_RX_POS (0)
#define SPIF_SCKR_DL_MODE_SEL	(0x1 << 20)
#define SPIF_CLK_SCKOUT_SRC_SEL	(0x1 << 26)

/* SPIF Interrupt status Register Bit Fields & Masks,default value:0x0000_0000 */
#define DMA_TRANS_DONE_INT	(0x1 << 24)

/* SPIF Trans Phase Configure Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_RX_TRANS_EN        (0x1 << 8)
#define SPIF_TX_TRANS_EN        (0x1 << 12)
#define SPIF_DUMMY_TRANS_EN     (0x1 << 16)
#define SPIF_MODE_TRANS_EN      (0x1 << 20)
#define SPIF_ADDR_TRANS_EN      (0x1 << 24)
#define SPIF_CMD_TRANS_EN       (0x1 << 28)

/* SPIF Trans Configure2 Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_DATA_TRANS_POS     (0)
#define SPIF_MODE_TRANS_POS     (4)
#define SPIF_ADDR_TRANS_POS     (8)
#define SPIF_CMD_TRANS_POS      (12)
#define SPIF_MODE_OPCODE_POS    (16)
#define SPIF_CMD_OPCODE_POS     (24)

/* SPIF Trans Number Register Bit Fields & Masks,default value:0x0000_0000 */
#define SPIF_ADDR_SIZE_MODE     (0x1 << 24)
#define SPIF_DUMMY_NUM_POS      (16)
#define SPIF_DATA_NUM_POS	(0)

/* SPIF DMA Control Register Bit Fields & Masks,default value:0x0000_0000 */
#define CFG_DMA_START		(1 << 0)
#define DMA_DESCRIPTOR_LEN	(32 << 4)

/* DMA descriptor0 Bit Fields & Masks */
#define HBURST_SINGLE_TYPE	(0x0 << 4)
#define HBURST_INCR4_TYPE	(0x3 << 4)
#define HBURST_INCR8_TYPE	(0x5 << 4)
#define HBURST_INCR16_TYPE	(0x7 << 4)
#define HBURST_TYPE		(0x7 << 4)
#define DMA_RW_PROCESS		(0x1 << 1) /* 0:Read  1:Write */
#define DMA_FINISH_FLASG	(0x1 << 0) /* The Last One Descriptor */

/* DMA descriptor1 Bit Fields & Masks */
#define DMA_BLK_LEN_8B		(0x0 << 16)
#define DMA_BLK_LEN_16B		(0x1 << 16)
#define DMA_BLK_LEN_32B		(0x2 << 16)
#define DMA_BLK_LEN_64B		(0x3 << 16)
#define DMA_BLK_LEN		(0xffff << 16)
#define DMA_DATA_LEN_POS	(0)
#define DMA_DATA_LEN		(0xffff)

/* DMA descriptor7 Bit Fields & Masks */

#define SPIF_DES_NORMAL_EN	(0x1 << 28)

#define SPIF_OCTAL_MODE		(8)
#define SPIF_QUAD_MODE		(4)
#define SPIF_DUEL_MODE		(2)
#define SPIF_SINGLE_MODE	(1)

#define SPIF_MIN_TRANS_NUM	(8)

#define NOR_PAGE_SIZE		(256)

#define SAMP_MODE_DL_DEFAULT	0xaaaaffff
struct sunxi_spif_slave {
	struct spi_slave	slave;
	uint32_t		max_hz;
	uint32_t		mode;
	int			cs_bitmap;/* cs0- 0x1; cs1-0x2, cs0&cs1-0x3. */
	uint32_t		base_addr;
	unsigned int		right_sample_delay;
	unsigned int		right_sample_mode;
	unsigned int		rx_dtr_en;
	unsigned int		tx_dtr_en;
};

struct spif_descriptor_op {
	u32 hburst_rw_flag;
	u32 block_data_len;
	u32 data_addr;
	u32 next_des_addr;
	u32 trans_phase;//0x20
	u32 flash_addr;//0x24
	u32 cmd_mode_buswidth;//0x28
	u32 addr_dummy_data_count;//0x2c
};

struct sunxi_spif_slave *get_sspif(void);
int spif_xfer(struct spi_slave *slave, struct spif_descriptor_op *spif_op);
void spif_update_right_delay_para(struct mtd_info *mtd);
int spif_set_right_delay_para(struct mtd_info *mtd);
void spif_init_dtr(u32 flags);

/**
 * Claim the bus and prepare it for communication with a given slave.
 *
 * This must be called before doing any transfers with a SPI slave. It
 * will enable and initialize any SPI hardware as necessary, and make
 * sure that the SCK line is in the correct idle state. It is not
 * allowed to claim the same bus for several slaves without releasing
 * the bus in between.
 *
 * @slave:	The SPI slave
 *
 * Returns: 0 if the bus was claimed successfully, or a negative value
 * if it wasn't.
 */
int spif_claim_bus(struct spi_slave *slave);

/**
 * Free any memory associated with a SPI slave.
 *
 * @slave:	The SPI slave
 */
void spif_free_slave(struct spi_slave *slave);

/**
 * Set up communications parameters for a SPI slave.
 *
 * This must be called once for each slave. Note that this function
 * usually doesn't touch any actual hardware, it only initializes the
 * contents of spi_slave so that the hardware can be easily
 * initialized later.
 *
 * @bus:	Bus ID of the slave chip.
 * @cs:		Chip select ID of the slave chip on the specified bus.
 * @max_hz:	Maximum SCK rate in Hz.
 * @mode:	Clock polarity, clock phase and other parameters.
 *
 * Returns: A spi_slave reference that can be used in subsequent SPI
 * calls, or NULL if one or more of the parameters are not supported.
 */
struct spi_slave *spif_setup_slave(unsigned int bus, unsigned int cs,
		unsigned int max_hz, unsigned int mode);

#endif
