// SPDX-License-Identifier: GPL-2.0

/*
 * All functions under this file are transfer interface to init flash on
 * allwinner way
 */

#include <linux/kernel.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/mtd.h>
#include <linux/printk.h>
#include <linux/sizes.h>
#include <linux/libfdt.h>
#include <common.h>
#include <stdbool.h>
#include <spi-mem.h>
#include <spi.h>
#include <fdt_support.h>
#include <linux/mtd/aw-spinand.h>
#include <private_toc.h>
#include "sprite_verify.h"

#include <linux/mtd/aw-ubi.h>
#include "sunxi-spinand.h"
#include <spare_head.h>

#define UBOOT_START_BLOCK_BIGNAND 4
#define UBOOT_START_BLOCK_SMALLNAND 8
#define NAND_BOOT0_BLK_START 0
#define MAX_MTD_PART0_BLKS 50
#define SECURE_STORAGE_BLKS 8
#define MAX_UBOOT_END_BLK (MAX_MTD_PART0_BLKS - SECURE_STORAGE_BLKS)
#define VALID_PAGESIZE_FOR_BOOT0 2048

static int enable_spinand = -1;

extern int get_boot_storage_type_ext(void);
extern int get_boot_work_mode(void);

int spinand_init_mtd_info(struct ubi_mtd_info *mtd_info)
{
	struct aw_spinand *spinand = get_spinand();
	struct mtd_info *mtd = spinand_to_mtd(spinand);
	int i;

	mtd_info->last_partno = -1;
	mtd_info->blksize = mtd->erasesize;
	mtd_info->pagesize = mtd->writesize;
	mtd_info->total_bytes = mtd->size;
	mtd_info->part_cnt = sunxi_get_mtd_num_parts();

	for (i = 0; i < mtd_info->part_cnt; i++) {
		const char *part_name = sunxi_get_mtdparts_name(i);
		struct ubi_mtd_part *part = &mtd_info->part[i];

		part->partno = i;
		strncpy(part->name, part_name, 16);
		part->offset = sunxi_get_mtdpart_offset(i);
		part->bytes = sunxi_get_mtdpart_size(i);
	}
	return 0;
}

inline int aw_spinand_mtd_read(unsigned int start, unsigned int sects, void *buf)
{
	struct aw_spinand *spinand = get_spinand();
	struct mtd_info *mtd = spinand_to_mtd(spinand);
	size_t block_offset, retlen, len = sects << 9, op_len;
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;
	struct aw_spinand_info *info = chip->info;
	int block_size = info->phy_block_size(chip);
	struct aw_spinand_chip_request req = {0};
	int ret;
	req.block = start / block_size;
	req.page = 0;

	do {
		if (req.page == 0) {
			/* must check bad and do erase for new block */
			ret = ops->phy_is_bad(chip, &req);
			if (ret == true) {
				pr_info("skip bad blk %d for boot, try next blk %d\n",
						req.block, req.block + 1);
				req.block++;
				start += block_size;
				continue;
			}
			if (ret) {
				pr_err("erase blk %d failed\n", req.block);
				return ret;
			}
		}

		block_offset = start & (block_size - 1);
		op_len = min(len, block_size - block_offset);

		ret = mtd_read(mtd, start, op_len, &retlen, buf);
		if (ret) {
			pr_err("** MTD read error\n");
			return -1;
		}

		if (!((start + retlen) % block_size)) {
			start += retlen;
			req.block++;
		}

		buf += retlen;
		len -= retlen;
	} while (len);

	return 0;
}

inline int aw_spinand_mtd_write(unsigned int start, unsigned int sects, void *buf)
{
	struct aw_spinand *spinand = get_spinand();
	struct mtd_info *mtd = spinand_to_mtd(spinand);
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;
	struct aw_spinand_info *info = chip->info;
	unsigned int erase_size = info->phy_block_size(chip);
	size_t retlen;
	int ret = 0;
	u32 erase_align_addr = 0;
	u32 erase_align_ofs = 0;
	u32 erase_align_size = 0;
	u32 data_len = sects << 9;
	unsigned char *align_buf = NULL;
	struct erase_info einfo;
	struct aw_spinand_chip_request req = {0};
	req.block = start / erase_size;
	req.page = 0;

	align_buf = malloc_align(erase_size, 64);
	if (!align_buf) {
		printf("%s: malloc error\n", __func__);
		return 0;
	}

	if (start % erase_size) {
		if (req.page == 0) {
retry:
			/* must check bad and do erase for new block */
			ret = ops->phy_is_bad(chip, &req);
			if (ret == true) {
				pr_info("skip bad blk %d for boot, try next blk %d\n",
						req.block, req.block + 1);
				req.block++;
				start += erase_size;
				goto retry;
			}
			if (ret) {
				pr_err("erase blk %d failed\n", req.block);
				return ret;
			}
		}

		erase_align_addr = (start / erase_size) * erase_size;
		/*
		|-------|-------|---------|
		     |<----data---->|
		    offset          end
		*/
		erase_align_ofs = start % erase_size;
		erase_align_size = erase_size - erase_align_ofs;
		erase_align_size = erase_align_size > data_len ?
						data_len : erase_align_size;
		ret = mtd->_read(mtd, erase_align_addr, erase_size,
				&retlen, align_buf);
		if (ret)
			goto exit;
		if (retlen != erase_size) {
			ret = -EIO;
			goto exit;
		}

		/* Erase the entire sector */
		einfo.addr = erase_align_addr;
		einfo.len = erase_size;
		ret = mtd->_erase(mtd, &einfo);
		if (ret) {
			printf("mtdblock: erase of region [0x%llx, 0x%llx] "
				     "on \"%s\" failed\n",
			einfo.addr, einfo.len, mtd->name);
			goto exit;
		}

		/*fill data to write*/
		memcpy(align_buf + erase_align_ofs, buf, erase_align_size);

		ret = mtd->_write(mtd, erase_align_addr, erase_size,
				&retlen, buf);
		if (ret)
			goto exit;
		if (retlen != erase_size) {
			ret = -EIO;
			goto exit;
		}

		/* update info */
		data_len -= erase_align_size;
		start += erase_align_size;
		buf += erase_align_size;
		req.block++;
	}

	while (data_len) {
		if (req.page == 0) {
			/* must check bad and do erase for new block */
			ret = ops->phy_is_bad(chip, &req);
			if (ret == true) {
				pr_info("skip bad blk %d for boot, try next blk %d\n",
						req.block, req.block + 1);
				req.block++;
				start += erase_size;
				continue;
			}
			if (ret) {
				pr_err("erase blk %d failed\n", req.block);
				return ret;
			}
		}

		if (data_len < erase_size) {
			ret = mtd->_read(mtd, start, erase_size,
					&retlen, align_buf);
			if (ret)
				goto exit;
			if (retlen != erase_size) {
				ret = -EIO;
				goto exit;
			}

			/* Erase the entire sector */
			einfo.addr = start;
			einfo.len = erase_size;
			ret = mtd->_erase(mtd, &einfo);
			if (ret) {
				printf("mtdblock: erase of region "
					"[0x%llx, 0x%llx] on \"%s\" failed\n",
				einfo.addr, einfo.len, mtd->name);
				goto exit;
			}

			/*fill data to write*/
			memcpy(align_buf, buf, data_len);

			ret = mtd->_write(mtd, start, erase_size,
					&retlen, buf);
			if (ret)
				goto exit;
			if (retlen != erase_size) {
				ret = -EIO;
				goto exit;
			}

			goto exit;
		}

		einfo.addr = start;
		einfo.len = erase_size;
		ret = mtd->_erase(mtd, &einfo);
		if (ret) {
			printf("mtdblock: erase of region [0x%llx, 0x%llx] "
				     "on \"%s\" failed\n",
			einfo.addr, einfo.len, mtd->name);
			goto exit;
		}

		ret = mtd->_write(mtd, start, erase_size, &retlen, buf);
		if (ret)
			goto exit;
		if (retlen != erase_size) {
			ret = -EIO;
			goto exit;
		}

		/* update info */
		data_len -= erase_size;
		start += erase_size;
		buf += erase_size;
		req.block++;
	}

exit:
	free_align(align_buf);
	return ret;
}

static int do_erase_block(int start_blk, int end_blk)
{
	struct aw_spinand *spinand = get_spinand();
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;
	struct aw_spinand_chip_request req = {0};
	int ret, tmp = 0;

	pr_info("erase blk %d to blk %d\n", start_blk, end_blk);
	for (; start_blk < end_blk; start_blk++) {
		req.block = start_blk;

		ret = ops->phy_is_bad(chip, &req);
		if (ret < 0) {
			pr_err("phy is bad error with %d back\n", ret);
			continue;
		} else if (ret == true) {
			pr_info("blk %d is bad, skip to erase\n", start_blk);
			continue;
		}

		ret = ops->phy_erase_block(chip, &req);
		if (ret) {
			/* keep erase, do not stop */
			pr_err("erase blk %d failed with %d back\n",
					start_blk, ret);
			tmp = ret;
			ret = ops->phy_mark_bad(chip, &req);
			if (ret)
				pr_err("mark blk %d as bad failed\n", start_blk);
		}
	}

	ret = tmp;
	return ret;
}

void spinand_uboot_blknum(unsigned int *start, unsigned int *end)
{
	struct aw_spinand *spinand = get_spinand();
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	unsigned int blksize = info->phy_block_size(chip);
	unsigned int pagecnt = blksize / info->phy_page_size(chip);
	unsigned int _start, _end;

	/* small nand:block size < 1MB;  reserve 4M for uboot */
	if (blksize <= SZ_128K) {
		_start = UBOOT_START_BLOCK_SMALLNAND;
		_end = _start + 32;
	} else if (blksize <= SZ_256K) {
		_start = UBOOT_START_BLOCK_SMALLNAND;
		_end = _start + 16;
	} else if (blksize <= SZ_512K) {
		_start = UBOOT_START_BLOCK_SMALLNAND;
		_end = _start + 8;
	} else if (blksize <= SZ_1M && pagecnt <= 128) { //1M
		_start = UBOOT_START_BLOCK_SMALLNAND;
		_end = _start + 4;
	/* big nand;  reserve at least 20M for uboot */
	} else if (blksize <= SZ_1M && pagecnt > 128) {
		_start = UBOOT_START_BLOCK_BIGNAND;
		_end = _start + 20;
	} else if (blksize <= SZ_2M) {
		_start = UBOOT_START_BLOCK_BIGNAND;
		_end = _start + 10;
	} else {
		_start = UBOOT_START_BLOCK_BIGNAND;
		_end = _start + 8;
	}

	if (CONFIG_AW_MTD_SPINAND_UBOOT_BLKS > 0)
		_end = _start + CONFIG_AW_MTD_SPINAND_UBOOT_BLKS;

	if (_end > MAX_UBOOT_END_BLK) {
		pr_warn("uboot end blocks %u is larger than max %u, resize!\n",
				_end, MAX_UBOOT_END_BLK);
		_end = MAX_UBOOT_END_BLK;
	}

	if (start)
		*start = _start;
	if (end)
		*end = _end;
}

static int spinand_mtd_erase_boot(void)
{
	unsigned int start, end;

	start = 0;
	spinand_uboot_blknum(NULL, &end);

	/* [start, end) */
	return do_erase_block(start, end);
}

static int spinand_mtd_erase_data(void)
{
	unsigned int start, end;
	struct aw_spinand *spinand = get_spinand();
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;

	spinand_uboot_blknum(NULL, &start);
	start += AW_SPINAND_RESERVED_PHY_BLK_FOR_SECURE_STORAGE;
	end = info->total_size(chip) / info->phy_block_size(chip);

	/* [start, end) */
	return do_erase_block(start, end);
}

int spinand_mtd_erase(int flag)
{
	spinand_mtd_erase_boot();
	if (flag)
		spinand_mtd_erase_data();
	return 0;
}

int spinand_mtd_force_erase(void)
{
	return spinand_mtd_erase(1);
}

/**
 * do download boot data to flash
 *
 * @startblk: the block to downlaod
 * @endblk: the end block to downlaod [start, end)
 * @pagesize: data size to write to each page, 0 means the whole page
 * @buf: data buffer
 * @len: length of buf
 *
 * return the blocks count written including the bad blocks.
 * return negative number if error.
 */
static int download_boot(unsigned int startblk, unsigned int endblk,
		unsigned int pagesize, void *buf, unsigned int len)
{
	struct aw_spinand *spinand = get_spinand();
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_chip_ops *ops = chip->ops;
	struct aw_spinand_info *info = chip->info;
	struct aw_spinand_phy_info *pinfo = info->phy_info;
	struct aw_spinand_chip_request req = {0};
	unsigned int pagecnt = pinfo->PageCntPerBlk;
	unsigned int written_blks = 0;
	int ret, pgindex = 0;

	/* check boundary */
	pagesize = pagesize ? pagesize : info->phy_page_size(chip);
	if (pagesize > info->phy_page_size(chip)) {
		pagesize = info->phy_page_size(chip);
		pr_warn("reset download boot size for each page to %d\n", pagesize);
	}
	/* download */
	req.block = startblk;
	req.page = 0;
	do {
		if (req.page == 0) {
			/* must check bad and do erase for new block */
			ret = ops->phy_is_bad(chip, &req);
			if (ret == true) {
				pr_info("skip bad blk %d for boot, try next blk %d\n",
						req.block, req.block + 1);
				req.block++;
				written_blks++;
				if (req.block >= endblk) {
					pr_info("achieve maximum blocks %d\n", endblk);
					return written_blks;
				}
				/* continue to check bad blk before erase */
				continue;
			}
			ret = ops->phy_erase_block(chip, &req);
			if (ret) {
				pr_err("erase blk %d failed\n", req.block);
				return ret;
			}
		}

		req.datalen = min(len, pagesize);
		/*
		 * only on the last time, datalen may be less than pagesize,
		 * to calculate the offset, we should use pagesize * pgindex
		 */
		req.databuf = buf + pgindex * pagesize;

		ret = ops->phy_write_page(chip, &req);
		if (ret) {
			pr_err("write boot to blk %d page %d failed\n",
					req.block, req.page);
			return ret;
		}
		if (req.page == 0)
			written_blks++;

		pgindex++;
		len -= req.datalen;
		req.page++;
		if (req.page >= pagecnt) {
			req.page = 0;
			req.block++;
		}
	} while (len > 0);

	return written_blks;
}

/*
 * this function is used to fix for aw boot0
 * we do not make a new way for ubi on boot0, to fix for the old way, we
 * have to fill boot_spinand_para_t for boot0 header
 */
int spinand_mtd_get_flash_info(void *data, unsigned int len)
{
	struct aw_spinand *spinand = get_spinand();
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	struct aw_spinand_phy_info *pinfo = info->phy_info;
	boot_spinand_para_t *boot_info = data;
	int spi_node, ret;
	unsigned int max_hz, uboot_start, uboot_end;

	spi_node = fdt_path_offset(working_fdt, "spi0/spi-nand");
	if (spi_node < 0) {
		pr_err("get spi-nand node from fdt failed\n");
		return -EINVAL;
	}

	ret = fdt_getprop_u32(working_fdt, spi_node, "spi-max-frequency", &max_hz);
	if (ret < 0) {
		pr_err("get spi-max-frequency from node of spi-nand failed\n");
		return -EINVAL;
	}
	spinand_uboot_blknum(&uboot_start, &uboot_end);

	/* nand information */
	boot_info->ChipCnt = 1;
	boot_info->ConnectMode = 1;
	boot_info->BankCntPerChip = 1;
	boot_info->DieCntPerChip = pinfo->DieCntPerChip;
	boot_info->PlaneCntPerDie = 2;
	boot_info->SectorCntPerPage = pinfo->SectCntPerPage;
	boot_info->ChipConnectInfo = 1;
	boot_info->PageCntPerPhyBlk = pinfo->PageCntPerBlk;
	boot_info->BlkCntPerDie = pinfo->BlkCntPerDie;
	boot_info->OperationOpt = pinfo->OperationOpt;
	boot_info->FrequencePar = max_hz / 1000 / 1000;
	boot_info->SpiMode = 0;
	info->nandid(chip, boot_info->NandChipId, 8);
	boot_info->pagewithbadflag = pinfo->BadBlockFlag;
	boot_info->MultiPlaneBlockOffset = 1;
	boot_info->MaxEraseTimes = pinfo->MaxEraseTimes;
	/* there is no metter what max ecc bits is */
	boot_info->MaxEccBits = 4;
	boot_info->EccLimitBits = 4;

	/* others */
	boot_info->uboot_start_block = uboot_start;
	boot_info->uboot_next_block = uboot_end;
	boot_info->logic_start_block = uboot_end;
	boot_info->nand_specialinfo_page = 0;
	boot_info->nand_specialinfo_offset = 0;
	boot_info->physic_block_reserved = 0;

	if (chip->rx_bit != 4)
		boot_info->OperationOpt &= ~SPINAND_QUAD_READ;
	boot_info->sample_mode = spinand->right_sample_mode;
	boot_info->sample_delay = spinand->right_sample_delay;
	return 0;
}

int spinand_mtd_download_boot0(unsigned int len, void *buf)
{
	unsigned start, end;
	int ret = 0;

	start = NAND_BOOT0_BLK_START;
	/* start addr of uboot is the end addr of boot0 */
	spinand_uboot_blknum(&end, NULL);

	/* In general, size of boot0 is less than a block */
	while (start < end) {
		pr_info("download boot0 to block %d len %dK\n", start, len / SZ_1K);
		ret = download_boot(start, end, VALID_PAGESIZE_FOR_BOOT0,
				buf, len);
		if (ret <= 0) {
			pr_err("download boot0 to blk %d failed\n", start);
			break;
		} else if (ret > 0) {
			/* if grater than zero, means had written @ret blks */
			start += ret;
		} else {
			start++;
		}
	}

	return 0;
}

int spinand_mtd_download_uboot(unsigned int len, void *buf)
{
	struct aw_spinand *spinand = get_spinand();
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	unsigned int phy_blk_size;
	unsigned int start, end, blks_per_uboot;
	int blks_written = 0;

	/* [start, end) */
	spinand_uboot_blknum(&start, &end);
	phy_blk_size = info->phy_block_size(chip);
	blks_per_uboot = (len + phy_blk_size - 1) / phy_blk_size;

	if (end - start < blks_per_uboot) {
		pr_err("no enough space for at least one uboot\n");
		pr_err("block per uboot is %u, however the uboot blks range is only [%u,%u)\n",
				blks_per_uboot, start, end);
		return -ENOSPC;
	}

	pr_info("uboot blk range [%d-%d)\n", start, end);
	if (len % info->page_size(chip)) {
		pr_err("len (%d) of uboot must align to pagesize %d\n",
				len, info->page_size(chip));
		return -EINVAL;
	}

	while (start + blks_per_uboot <= end) {
		pr_info("download uboot to block %d (%d blocks) len %dK\n",
				start, blks_per_uboot, len / SZ_1K);
		blks_written = download_boot(start, end, 0, buf, len);
		if (blks_written <= 0) {
			pr_err("download uboot to blk %d failed\n", start);
			return blks_written;
		}
		if (blks_written < blks_per_uboot) {
			pr_err("something error, written %d blks but wanted %d blks\n",
					blks_written, blks_per_uboot);
			return -EINVAL;
		}
		start += blks_written;
	}

	return 0;
}

int spinand_mtd_upload_uboot(void *buf, unsigned int len)
{
	struct aw_spinand *spinand = get_spinand();
	struct aw_spinand_chip *chip = spinand_to_chip(spinand);
	struct aw_spinand_info *info = chip->info;
	sbrom_toc1_head_info_t *toc1;
	int block_size = info->phy_block_size(chip);
	unsigned int start, end, check_sum, blks_per_uboot;
	int ret;

	blks_per_uboot = (len + block_size - 1) / block_size;

	spinand_uboot_blknum(&start, &end);	/* start and end of the bootpackage */

	ret = aw_spinand_mtd_read(start * block_size, len/512, buf);	/* read bootpackage  */
	if (ret) {
		pr_err("fail to read first bootpackage, read backup bootpackage\n");
	} else {
		toc1 = (sbrom_toc1_head_info_t *)buf;
		check_sum = sunxi_sprite_generate_checksum(buf,
				toc1->valid_len, toc1->add_sum);
		if (check_sum == toc1->add_sum) {
			pr_info("read bootpackage finished\n");
			return 0;
		}
	}

	pr_info("first bootpackaeg is damaged, read backup bootpackage\n");
	start += blks_per_uboot;
	while (start + blks_per_uboot <= end) {	/* read backup bootpackage */
		ret = aw_spinand_mtd_read(start * block_size, len/512, (void *)buf);
		if (ret) {
			start += blks_per_uboot;
			continue;
		}
		toc1 = (sbrom_toc1_head_info_t *)buf;
		check_sum = sunxi_sprite_generate_checksum(buf,
				toc1->valid_len, toc1->add_sum);
		if (check_sum == toc1->add_sum) {
			pr_info("read backup bootpackage finished\n");
			return 0;
		}
		start += blks_per_uboot;
	}
	pr_err("mtd_upload_uboot error\n");
	return -1;
}

int spinand_mtd_flush(void)
{
	struct aw_spinand *spinand = get_spinand();
	struct mtd_info *mtd = spinand_to_mtd(spinand);

	mtd_sync(mtd);
	return 0;
}

int spinand_mtd_write_end(void)
{
	return spinand_mtd_flush_last_volume();
}

int spinand_mtd_init(void)
{
	struct spi_slave *slave = NULL;
	struct udevice dev = {0};
	int ret;
#ifndef CONFIG_AW_SPINAND_NONSTANDARD_SPI_DRIVER
	struct aw_spinand *spinand;
	int spi_node;
	unsigned int max_hz, cs = 0, spi_mode = 0;

	spinand = get_spinand();
	if (spinand) {
		pr_info("mtd spinand already init\n");
		return 0;
	}

	spi_node = fdt_path_offset(working_fdt, "spi0/spi-nand");
	if (spi_node < 0) {
		pr_err("get spi-nand node from fdt failed\n");
		return -EINVAL;
	}

	ret = fdt_getprop_u32(working_fdt, spi_node, "spi-max-frequency", &max_hz);
	if (ret < 0) {
		pr_err("get spi-max-frequency from node of spi-nand failed\n");
		return -EINVAL;
	}
	pr_info("set spi-nand frequency: %u Hz\n", max_hz);

	slave = spi_setup_slave(0, cs, max_hz, spi_mode);
	if (!slave) {
		pr_err("setup spi slave failed\n");
		return -EINVAL;
	}
#endif

	dev.parent_priv = slave;
	dev.uclass_priv = NULL;
	ret = aw_spinand_probe(&dev);
#ifndef CONFIG_AW_SPINAND_NONSTANDARD_SPI_DRIVER
	if (ret)
		spi_release_bus(slave);
#endif
	return ret;
}

int spinand_mtd_exit(void)
{
	struct aw_spinand *spinand = get_spinand();

	if (spinand) {
		aw_spinand_exit(spinand);
#ifndef CONFIG_AW_SPINAND_NONSTANDARD_SPI_DRIVER
		struct aw_spinand_chip *chip = spinand_to_chip(spinand);
		spi_release_bus(chip->slave);
#endif
	}
	return 0;
}

bool support_spinand(void)
{
	int spi_node = 0;


	if (enable_spinand != -1)
			return enable_spinand;

	if (get_boot_work_mode() == WORK_MODE_BOOT &&
			get_boot_storage_type_ext() == STORAGE_SPI_NAND) {
		spi_node = fdt_path_offset(working_fdt, "spi0/spi-nand");
		if (spi_node < 0) {
			pr_err("get spi-nand node from fdt failed\n");
			return false;
		}
#ifdef CONFIG_SUNXI_UBIFS
		enable_spinand = true;
#else
		enable_spinand = false;
#endif
	} else if (get_boot_work_mode() != WORK_MODE_BOOT) {

		spi_node = fdt_path_offset(working_fdt, "spi0/spi-nand");
		if (spi_node < 0) {
			pr_err("get spi-nand node from fdt failed\n");
			return false;
		}
#ifdef CONFIG_SUNXI_UBIFS
		enable_spinand = true;
#else
		enable_spinand = false;
#endif
	}
	return enable_spinand;
}

void disable_spinand(void)
{
	enable_spinand = false;
}

unsigned spinand_mtd_size(void)
{
	return spinand_ubi_user_volumes_size();
}

int spinand_mtd_secure_storage_read(int item, char *buf, unsigned int len)
{
	struct aw_spinand *spinand = get_spinand();

	return aw_spinand_secure_storage_read(&spinand->sec_sto, item, buf, len);
}

int spinand_mtd_secure_storage_write(int item, char *buf, unsigned int len)
{
	struct aw_spinand *spinand = get_spinand();

	return aw_spinand_secure_storage_write(&spinand->sec_sto, item, buf, len);
}
