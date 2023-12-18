/*
 * (C) Copyright 2022-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * lujianliang <lujianliang@allwinnertech.com>
 */

#ifndef __BOOT0_SPI_MEM_H
#define __BOOT0_SPI_MEM_H

#define BIT(nr)			(1UL << (nr))
/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_ULL(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
#define GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h, l) \
	(((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

#define SPI_MEM_OP_CMD(__opcode, __buswidth)			\
	{							\
		.buswidth = __buswidth,				\
		.opcode = __opcode,				\
	}

#define SPI_MEM_OP_NO_CMD	{ }

#define SPI_MEM_OP_ADDR(__nbytes, __val, __buswidth)		\
	{							\
		.nbytes = __nbytes,				\
		.val = __val,					\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_ADDR	{ }

#define SPI_MEM_OP_MODE(__val, __buswidth)                      \
       {                                                        \
		.val = __val,                                   \
		.buswidth = __buswidth,                         \
       }

#define SPI_MEM_OP_NO_MODE     { }

#define SPI_MEM_OP_DUMMY(__cycle, __buswidth)			\
	{							\
		.cycle = __cycle,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_DUMMY	{ }

#define SPI_MEM_OP_DATA_IN(__nbytes, __buf, __buswidth)		\
	{							\
		.dir = SPI_MEM_DATA_IN,				\
		.nbytes = __nbytes,				\
		.buf.in = __buf,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_DATA_OUT(__nbytes, __buf, __buswidth)	\
	{							\
		.dir = SPI_MEM_DATA_OUT,			\
		.nbytes = __nbytes,				\
		.buf.out = __buf,				\
		.buswidth = __buswidth,				\
	}

#define SPI_MEM_OP_NO_DATA	{ }

/**
 * enum spi_mem_data_dir - describes the direction of a SPI memory data
 *			   transfer from the controller perspective
 * @SPI_MEM_DATA_IN: data coming from the SPI memory
 * @SPI_MEM_DATA_OUT: data sent the SPI memory
 */
enum spi_mem_data_dir {
	SPI_MEM_DATA_IN,
	SPI_MEM_DATA_OUT,
};

/**
 * struct spi_mem_op - describes a SPI memory operation
 * @cmd.buswidth: number of IO lines used to transmit the command
 * @cmd.opcode: operation opcode
 * @addr.nbytes: number of address bytes to send. Can be zero if the operation
 *		 does not need to send an address
 * @addr.buswidth: number of IO lines used to transmit the address cycles
 * @addr.val: address value. This value is always sent MSB first on the bus.
 *	      Note that only @addr.nbytes are taken into account in this
 *	      address value, so users should make sure the value fits in the
 *	      assigned number of bytes.
 * @dummy.nbytes: number of dummy bytes to send after an opcode or address. Can
 *		  be zero if the operation does not require dummy bytes
 * @dummy.buswidth: number of IO lanes used to transmit the dummy bytes
 * @data.buswidth: number of IO lanes used to send/receive the data
 * @data.dir: direction of the transfer
 * @data.buf.in: input buffer
 * @data.buf.out: output buffer
 */
struct spi_mem_op {
	struct {
		u8 buswidth;
		u8 opcode;
	} cmd;

	struct {
		u8 nbytes;
		u8 buswidth;
		u64 val;
	} addr;

	struct {
		u8 buswidth;
		const void *val;
	} mode;

	struct {
		u8 cycle;
		u8 buswidth;
	} dummy;

	struct {
		u8 buswidth;
		enum spi_mem_data_dir dir;
		unsigned int nbytes;
		/* buf.{in,out} must be DMA-able. */
		union {
			void *in;
			const void *out;
		} buf;
	} data;
};

#define SPI_MEM_OP(__cmd, __addr, __mode, __dummy, __data)	\
	{							\
		.cmd = __cmd,					\
		.addr = __addr,					\
		.mode = __mode,                                 \
		.dummy = __dummy,				\
		.data = __data,					\
	}

/* Supported SPI protocols */
#define SNOR_PROTO_INST_MASK	GENMASK(23, 16)
#define SNOR_PROTO_INST_SHIFT	16
#define SNOR_PROTO_INST(_nbits)	\
	((((unsigned long)(_nbits)) << SNOR_PROTO_INST_SHIFT) & \
	 SNOR_PROTO_INST_MASK)

#define SNOR_PROTO_ADDR_MASK	GENMASK(15, 8)
#define SNOR_PROTO_ADDR_SHIFT	8
#define SNOR_PROTO_ADDR(_nbits)	\
	((((unsigned long)(_nbits)) << SNOR_PROTO_ADDR_SHIFT) & \
	 SNOR_PROTO_ADDR_MASK)

#define SNOR_PROTO_DATA_MASK	GENMASK(7, 0)
#define SNOR_PROTO_DATA_SHIFT	0
#define SNOR_PROTO_DATA(_nbits)	\
	((((unsigned long)(_nbits)) << SNOR_PROTO_DATA_SHIFT) & \
	 SNOR_PROTO_DATA_MASK)

#define SNOR_PROTO_IS_DTR	BIT(24)	/* Double Transfer Rate */

#define SNOR_PROTO_STR(_inst_nbits, _addr_nbits, _data_nbits)	\
	(SNOR_PROTO_INST(_inst_nbits) |				\
	 SNOR_PROTO_ADDR(_addr_nbits) |				\
	 SNOR_PROTO_DATA(_data_nbits))
#define SNOR_PROTO_DTR(_inst_nbits, _addr_nbits, _data_nbits)	\
	(SNOR_PROTO_IS_DTR |					\
	 SNOR_PROTO_STR(_inst_nbits, _addr_nbits, _data_nbits))

enum spi_nor_protocol {
	SNOR_PROTO_1_1_1 = SNOR_PROTO_STR(1, 1, 1),
	SNOR_PROTO_1_1_2 = SNOR_PROTO_STR(1, 1, 2),
	SNOR_PROTO_1_1_4 = SNOR_PROTO_STR(1, 1, 4),
	SNOR_PROTO_1_1_8 = SNOR_PROTO_STR(1, 1, 8),
	SNOR_PROTO_1_2_2 = SNOR_PROTO_STR(1, 2, 2),
	SNOR_PROTO_1_4_4 = SNOR_PROTO_STR(1, 4, 4),
	SNOR_PROTO_1_8_8 = SNOR_PROTO_STR(1, 8, 8),
	SNOR_PROTO_2_2_2 = SNOR_PROTO_STR(2, 2, 2),
	SNOR_PROTO_4_4_4 = SNOR_PROTO_STR(4, 4, 4),
	SNOR_PROTO_8_8_8 = SNOR_PROTO_STR(8, 8, 8),

	SNOR_PROTO_1_1_1_DTR = SNOR_PROTO_DTR(1, 1, 1),
	SNOR_PROTO_1_2_2_DTR = SNOR_PROTO_DTR(1, 2, 2),
	SNOR_PROTO_1_4_4_DTR = SNOR_PROTO_DTR(1, 4, 4),
	SNOR_PROTO_1_8_8_DTR = SNOR_PROTO_DTR(1, 8, 8),
};

static inline bool spi_nor_protocol_is_dtr(enum spi_nor_protocol proto)
{
	return !!(proto & SNOR_PROTO_IS_DTR);
}

static inline u8 spi_nor_get_protocol_inst_nbits(enum spi_nor_protocol proto)
{
	return ((unsigned long)(proto & SNOR_PROTO_INST_MASK)) >>
		SNOR_PROTO_INST_SHIFT;
}

static inline u8 spi_nor_get_protocol_addr_nbits(enum spi_nor_protocol proto)
{
	return ((unsigned long)(proto & SNOR_PROTO_ADDR_MASK)) >>
		SNOR_PROTO_ADDR_SHIFT;
}

static inline u8 spi_nor_get_protocol_data_nbits(enum spi_nor_protocol proto)
{
	return ((unsigned long)(proto & SNOR_PROTO_DATA_MASK)) >>
		SNOR_PROTO_DATA_SHIFT;
}

int spi_mem_exec_op(const struct spi_mem_op *op);
#endif
