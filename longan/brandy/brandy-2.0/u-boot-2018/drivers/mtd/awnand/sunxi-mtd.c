/*
 * (C) Copyright 2022-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * lujianliang <lujianliang@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#ifdef CONFIG_AW_MTD_SPINAND
#include <linux/mtd/aw-spinand.h>
#endif
#ifdef CONFIG_AW_MTD_RAWNAND
#include <linux/mtd/aw-rawnand.h>
#endif

int mtd_nand_write(unsigned int start, unsigned int sectors, void *buf)
{
	int ret = -EINVAL;

#ifdef CONFIG_AW_MTD_SPINAND
	if (support_spinand())
		return aw_spinand_mtd_write(start, sectors, buf);
#endif

#ifdef CONFIG_AW_MTD_RAWNAND
	if (support_rawnand())
		return aw_rawnand_phy_write(start, sectors, buf);
#endif
	return ret;
}

int mtd_nand_read(unsigned int start, unsigned int sectors, void *buf)
{
	int ret = -EINVAL;

#ifdef CONFIG_AW_MTD_SPINAND
	if (support_spinand())
		return aw_spinand_mtd_read(start, sectors, buf);
#endif

#ifdef CONFIG_AW_MTD_RAWNAND
	if (support_rawnand())
		return aw_rawnand_phy_read(start, sectors, buf);
#endif
	return ret;
}

