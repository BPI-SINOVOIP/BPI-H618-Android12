/*
 * (C) Copyright 2022-2025
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 *
 * lujianliang <lujianliang@allwinnertech.com>
 * SPDX-License-Identifier:     GPL-2.0+
 */


#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <spi.h>
#include <spi_flash.h>

#include "../../spi/spif-sunxi.h"

static int spif_flash_probe_slave(struct spi_flash *flash)
{
	struct spi_slave *spi = flash->spi;
	int ret;

	/* Setup spi_slave */
	if (!spi) {
		printf("SF: Failed to set up slave\n");
		return -ENODEV;
	}

	/* Claim spi bus */
	ret = spif_claim_bus(spi);
	if (ret) {
		debug("SF: Failed to claim SPI bus: %d\n", ret);
		return ret;
	}

	return spi_nor_scan(flash);
}

struct spi_flash *spif_flash_probe(unsigned int busnum, unsigned int cs,
				  unsigned int max_hz, unsigned int spi_mode)
{
	struct spi_slave *bus;
	struct spi_flash *flash;

	bus = spif_setup_slave(busnum, cs, max_hz, spi_mode);
	if (!bus)
		return NULL;

	/* Allocate space if needed (not used by sf-uclass */
	flash = calloc(1, sizeof(*flash));
	if (!flash) {
		debug("SF: Failed to allocate spi_flash\n");
		return NULL;
	}

	flash->spi = bus;
	if (spif_flash_probe_slave(flash)) {
		spif_free_slave(bus);
		free(flash);
		return NULL;
	}

	return flash;
}

void spif_flash_free(struct spi_flash *flash)
{
	spif_free_slave(flash->spi);
	free(flash);
}

