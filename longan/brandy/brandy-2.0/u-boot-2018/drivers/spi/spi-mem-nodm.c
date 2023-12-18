// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/
 */

#include <spi.h>
#include <spi-mem.h>
#include "spif-sunxi.h"

#ifdef CONFIG_SUNXI_SPIF
static int spif_select_buswidth(u32 buswidth)
{
	int width = 0;

	switch (buswidth) {
	case SPIF_SINGLE_MODE:
		width = 0;
		break;
	case SPIF_DUEL_MODE:
		width = 1;
		break;
	case SPIF_QUAD_MODE:
		width = 2;
		break;
	case SPIF_OCTAL_MODE:
		width = 3;
		break;
	default:
		printf("Parameter error with buswidth:%d\n", buswidth);
	}
	return width;
}


int spif_mem_exec_op(struct spi_slave *slave, const struct spi_mem_op *op)
{
	int ret;
	struct spif_descriptor_op spif_op __aligned(CONFIG_SYS_CACHELINE_SIZE);
	uint cache_buf[CONFIG_SYS_CACHELINE_SIZE]
		__aligned(CONFIG_SYS_CACHELINE_SIZE);

	memset(&spif_op, 0, sizeof(struct spif_descriptor_op));
	memset(cache_buf, 0, CONFIG_SYS_CACHELINE_SIZE);

	/* set hburst type */
	spif_op.hburst_rw_flag &= ~HBURST_TYPE;
	spif_op.hburst_rw_flag |= HBURST_INCR4_TYPE;

	/* the last one descriptor */
	spif_op.hburst_rw_flag |= DMA_FINISH_FLASG;
	/* set next des addr */

	/* set DMA block len mode */
	spif_op.block_data_len &= ~DMA_BLK_LEN;
	spif_op.block_data_len |= DMA_BLK_LEN_16B;

	/* dispose cmd */
	if (op->cmd.opcode) {
		spif_op.trans_phase |= SPIF_CMD_TRANS_EN;
		spif_op.cmd_mode_buswidth |= op->cmd.opcode << SPIF_CMD_OPCODE_POS;
		/* set cmd buswidth */
		spif_op.cmd_mode_buswidth |=
			spif_select_buswidth(op->cmd.buswidth) << SPIF_CMD_TRANS_POS;
		if (op->cmd.buswidth != 1)
			spif_op.cmd_mode_buswidth |=
				spif_select_buswidth(op->cmd.buswidth) <<
				SPIF_DATA_TRANS_POS;
	}

	/* dispose addr */
	if (op->addr.nbytes) {
		spif_op.trans_phase |= SPIF_ADDR_TRANS_EN;
		spif_op.flash_addr = op->addr.val;
		if (op->addr.nbytes == 4) //set 4byte addr mode
			spif_op.addr_dummy_data_count |= SPIF_ADDR_SIZE_MODE;
		/* set addr buswidth */
		spif_op.cmd_mode_buswidth |=
			spif_select_buswidth(op->addr.buswidth) <<
			SPIF_ADDR_TRANS_POS;
	}

	/* dispose mode */
	if (op->mode.val) {
		spif_op.trans_phase |= SPIF_MODE_TRANS_EN;
		spif_op.cmd_mode_buswidth |=
				*(u8 *)op->mode.val << SPIF_MODE_OPCODE_POS;
		/* set addr buswidth */
		spif_op.cmd_mode_buswidth |=
			spif_select_buswidth(op->mode.buswidth) <<
			SPIF_MODE_TRANS_POS;
	}

	/* dispose dummy */
	if (op->dummy.cycle) {
		spif_op.trans_phase |= SPIF_DUMMY_TRANS_EN;
		spif_op.addr_dummy_data_count |=
			(op->dummy.cycle << SPIF_DUMMY_NUM_POS);
	}

	/* dispose data */
	if (op->data.nbytes) {
		if (op->data.dir == SPI_MEM_DATA_IN) {
			spif_op.trans_phase |= SPIF_RX_TRANS_EN;
			if (op->data.nbytes < SPIF_MIN_TRANS_NUM)
				spif_op.data_addr = (u32)cache_buf;
			else
				spif_op.data_addr = (u32)op->data.buf.in;
			/* Write:1 DMA Write to dram */
			spif_op.hburst_rw_flag |= DMA_RW_PROCESS;
		} else {
			spif_op.trans_phase |= SPIF_TX_TRANS_EN;
			if (op->data.nbytes < NOR_PAGE_SIZE) {
				printf("Need to write by page\n");
				return -1;
			}
			spif_op.data_addr = (u32)op->data.buf.out;
			/* Read:0 DMA read for dram */
			spif_op.hburst_rw_flag &= ~DMA_RW_PROCESS;
		}
		if (op->data.nbytes < SPIF_MIN_TRANS_NUM &&
				op->data.dir == SPI_MEM_DATA_IN) {
			spif_op.addr_dummy_data_count |=
				SPIF_MIN_TRANS_NUM << SPIF_DATA_NUM_POS;
			spif_op.block_data_len |=
				SPIF_MIN_TRANS_NUM << SPIF_DATA_NUM_POS;
		} else {
			spif_op.addr_dummy_data_count |=
				op->data.nbytes << SPIF_DATA_NUM_POS;
			spif_op.block_data_len |=
				op->data.nbytes << SPIF_DATA_NUM_POS;
		}

		/* set data buswidth */
		spif_op.cmd_mode_buswidth |=
			spif_select_buswidth(op->data.buswidth) << SPIF_DATA_TRANS_POS;
	}

	ret = spif_xfer(slave, &spif_op);
	if (ret)
		return ret;

	if (op->data.nbytes < SPIF_MIN_TRANS_NUM &&
			op->data.dir == SPI_MEM_DATA_IN)
		memcpy((void *)op->data.buf.in,	(const void *)cache_buf,
					op->data.nbytes);
	return 0;
}
#endif /* CONFIG_SUNXI_SPIF */

int spi_mem_exec_op(struct spi_slave *slave,
		    const struct spi_mem_op *op)
{
#ifdef CONFIG_SUNXI_SPIF
	return spif_mem_exec_op(slave, op);
#else
	unsigned int pos = 0;
	const u8 *tx_buf = NULL;
	u8 *rx_buf = NULL;
	u8 *op_buf;
	int op_len;
	u32 flag;
	int ret;
	int i;
	/* convert the dummy cycles to the number of bytes */
	int dummy_nbytes = (op->dummy.cycle * op->dummy.buswidth) / 8;

	if (op->data.nbytes) {
		if (op->data.dir == SPI_MEM_DATA_IN)
			rx_buf = op->data.buf.in;
		else
			tx_buf = op->data.buf.out;
	}

	op_len = sizeof(op->cmd.opcode) + op->addr.nbytes + dummy_nbytes;
	op_buf = calloc(1, op_len);
	if (!op_buf) {
		ret = -1;
		goto err_op_buf;
	}

	ret = spi_claim_bus(slave);
	if (ret < 0)
		goto free_op_buf;

	op_buf[pos++] = op->cmd.opcode;

	if (op->addr.nbytes) {
		for (i = 0; i < op->addr.nbytes; i++)
			op_buf[pos + i] = op->addr.val >>
				(8 * (op->addr.nbytes - i - 1));

		pos += op->addr.nbytes;
	}

	if (op->dummy.cycle)
		memset(op_buf + pos, 0xff, dummy_nbytes);

	/* 1st transfer: opcode + address + dummy cycles */
	flag = SPI_XFER_BEGIN;
	/* Make sure to set END bit if no tx or rx data messages follow */
	if (!tx_buf && !rx_buf)
		flag |= SPI_XFER_END;

	ret = spi_xfer(slave, op_len * 8, op_buf, NULL, flag);
	if (ret)
		goto free_op_buf;

	/* 2nd transfer: rx or tx data path */
	if (tx_buf || rx_buf) {
		ret = spi_xfer(slave, op->data.nbytes * 8, tx_buf,
			       rx_buf, SPI_XFER_END);
		if (ret)
			goto free_op_buf;
	}

	spi_release_bus(slave);

	for (i = 0; i < pos; i++)
		debug("%02x ", op_buf[i]);
	debug("| [%dB %s] ",
	      tx_buf || rx_buf ? op->data.nbytes : 0,
	      tx_buf || rx_buf ? (tx_buf ? "out" : "in") : "-");
	for (i = 0; i < op->data.nbytes; i++)
		debug("%02x ", tx_buf ? tx_buf[i] : rx_buf[i]);
	debug("[ret %d]\n", ret);

free_op_buf:
	free(op_buf);
err_op_buf:
	if (ret < 0)
		return ret;

	return 0;
#endif  /* CONFIG_SUNXI_SPIF */
}

int spi_mem_adjust_op_size(struct spi_slave *slave,
			   struct spi_mem_op *op)
{
	unsigned int len;
	int dummy_nbytes = (op->dummy.cycle * op->dummy.buswidth) / 8;

	len = sizeof(op->cmd.opcode) + op->addr.nbytes + dummy_nbytes;
	if (slave->max_write_size && len > slave->max_write_size)
		return -EINVAL;

	if (op->data.dir == SPI_MEM_DATA_IN && slave->max_read_size)
		op->data.nbytes = min(op->data.nbytes,
				      slave->max_read_size);
	else if (slave->max_write_size)
		op->data.nbytes = min(op->data.nbytes,
				      slave->max_write_size - len);

	return 0;
}
