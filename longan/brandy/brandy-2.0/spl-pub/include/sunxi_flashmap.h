/*
 * Copyright (c) 2022-2024 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : mmc_flashmap.h
 * Description :
 * History :
 *    Author  :  <huangrongcun@allwinnertech.com>
 *    Date    : 2022/04/07
 *    Comment : mmc flash map
 */

#ifndef __SUNXI_FLASHMAP_h__
#define __SUNXI_FLASHMAP_h__

enum flash_type{
	FLASHMAP_SPI_NOR,
	FLASHMAP_SDMMC,
	FLASHMAP_TYPE_COUNT
};

int sunxi_flashmap_bootparam_start(enum flash_type flash_name);
int sunxi_flashmap_toc1_start(enum flash_type flash_name);
int sunxi_flashmap_toc1_bak_start(enum flash_type flash_name);
int sunxi_flashmap_info_dump(void);
#endif /*__SUNXI_FLASHMAP_h__*/