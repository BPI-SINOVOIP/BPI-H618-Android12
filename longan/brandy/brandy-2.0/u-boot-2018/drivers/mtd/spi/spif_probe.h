/*
 * (C) Copyright 2022-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * lujianliang <lujianliang@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __SPIF_PROBE_H__
#define __SPIF_PROBE_H__

struct spi_flash *spif_flash_probe(unsigned int busnum, unsigned int cs,
				  unsigned int max_hz, unsigned int spi_mode);
void spif_flash_free(struct spi_flash *flash);

#endif

