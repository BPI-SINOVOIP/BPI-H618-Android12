#ifndef __BOOT_PARAM_H
#define __BOOT_PARAM_H

#define BOOT_PARAM_MAGIC		"bootpara"
#define BOOT_PARAM_SIZE			4096

struct sunxi_boot_parameter_header {
	u8 magic[8]; //bootpara
	u32 version; // describe the region version
	u32 check_sum;
	u32 length;
	u8 reserved[12];
};

//---total len 是4K
struct sunxi_boot_param_region {
	struct sunxi_boot_parameter_header header;//32
	char sdmmc_info[256];
	char nand_info[256];
	char spiflash_info[256];
	char ddr_info[512];
	u8 reserved[2784];// = 4096 - sdmmc_size - nand_size - spi_size - ddr_size - 32
};

#endif
