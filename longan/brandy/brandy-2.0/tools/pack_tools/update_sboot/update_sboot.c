/* update.cpp : Defines the entry point for the console application.*/
/**/

#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "types.h"
#include "boot0_v2.h"
#include "check.h"
#include "script.h"
#include <ctype.h>
#include <unistd.h>
#include "commit.h"

#include "../update_scp/libfdt/libfdt_env.h"
#include "../update_scp/libfdt/fdt.h"
#include "../update_scp/libfdt/libfdt.h"
#include "../update_scp/libfdt/libfdt_internal.h"
#include "../include/type_def.h"

#define MAX_PATH (260)

#define SDMMC                  1
#define NOR                    2


#define FDT_PATH_MMCFLASH_MAP "/soc/sunxi_flashmap/sdmmc_map"
#define FDT_PATH_NORFLASH_MAP "/soc/sunxi_flashmap/nor_map"

/*__asm__(".symver memcpy ,memcpy@GLIBC_2.2.5");*/
void *script_file_decode(char *script_name);
int update_for_sboot(char *sboot_name, int storage_type);
/*------------------------------------------------------------------------------------------------------------*/
/**/
/* 函数说明*/
/**/
/**/
/* 参数说明*/
/**/
/**/
/* 返回值*/
/**/
/**/
/* 其他*/
/*    无*/
/**/
/*------------------------------------------------------------------------------------------------------------*/
int IsFullName(const char *FilePath)
{
	if (isalpha(FilePath[0]) && ':' == FilePath[1]) {
		return 1;
	} else {
		return 0;
	}
}
/*------------------------------------------------------------------------------------------------------------*/
/**/
/* 函数说明*/
/**/
/**/
/* 参数说明*/
/**/
/**/
/* 返回值*/
/**/
/**/
/* 其他*/
/*    无*/
/**/
/*------------------------------------------------------------------------------------------------------------*/
void GetFullPath(char *dName, const char *sName)
{
	char Buffer[MAX_PATH];

	if (IsFullName(sName)) {
		strcpy(dName, sName);
		return;
	}

	/* Get the current working directory: */
	if (getcwd(Buffer, MAX_PATH) == NULL) {
		perror("getcwd error");
		return;
	}
	sprintf(dName, "%s/%s", Buffer, sName);
}

/*------------------------------------------------------------------------------------------------------------*/
/**/
/* 函数说明*/
/**/
/**/
/* 参数说明*/
/**/
/**/
/* 返回值*/
/**/
/**/
/* 其他*/
/*    无*/
/**/
/*------------------------------------------------------------------------------------------------------------*/
void Usage(void)
{
	printf("\n");
	printf("Usage:\n");
	printf("update.exe script file path para file path\n\n");
}

char *probe_file_data(char *file_name, int *file_len)
{
	FILE *pfile;
	int   len;
	char *buffer;

	pfile = fopen(file_name, "rb");
	if (pfile == NULL) {
		printf("file %s cant be opened\n", file_name);

		return NULL;
	}
	fseek(pfile, 0, SEEK_END);
	len = ftell(pfile);

	buffer = malloc(len);
	if (buffer == NULL) {
		printf("buffer cant be malloc\n");
		fclose(pfile);

		return NULL;
	}

	memset(buffer, 0, len);

	fseek(pfile, 0, SEEK_SET);
	fread(buffer, len, 1, pfile);
	fclose(pfile);

	*file_len = len;

	return buffer;
}


int update_flashmap_for_boot0(sboot_file_head_t *sboot_head, const void *working_fdt, int storage_type)
{
	printf("update flash map\n");
	int nodeoffset;
	int boot_param = 0, uboot_start_sector = 0, uboot_bak_start_sector = 0;

	if (storage_type == SDMMC) {
		nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_MMCFLASH_MAP);
		if (nodeoffset < 0) {
			printf("error:no node sunxi_flashmap\n");
			return -1;
		}
	} else if (storage_type == NOR) {
		nodeoffset = fdt_path_offset(working_fdt, FDT_PATH_NORFLASH_MAP);
		if (nodeoffset < 0) {
			printf("error:no node sunxi_flashmap\n");
			return -1;
		}
	} else {
		printf("not support medium\n");
		return -1;
	}


	if (!fdtdec_get_is_enabled(working_fdt, nodeoffset)) {
		printf("FDT sunxi flash disable\n");
		return -1;
	}
	fdt_getprop_u32(working_fdt, nodeoffset, "boot_param_start", &boot_param);
	fdt_getprop_u32(working_fdt, nodeoffset, "uboot_start", &uboot_start_sector);
	fdt_getprop_u32(working_fdt, nodeoffset, "uboot_bak_start", &uboot_bak_start_sector);

	printf("medium(1:SDMMC,2:NOR):%d\nboot_param:%d\nuboot_start:%d\n", storage_type, boot_param, uboot_start_sector);
	sboot_head->flash_map.boot_param = boot_param;
	sboot_head->flash_map.uboot_start_sector = uboot_start_sector;
	sboot_head->flash_map.uboot_bak_start_sector = uboot_bak_start_sector;
	return 0;
}

int update_from_fdt(char *sboot_name, char *dtb_name, int storage_type)
{
	FILE *sboot_file = NULL;
	sboot_file_head_t *sboot_head;
	char *sboot_buf = NULL;
	int length      = 0;
	int ret = -1;

	char *working_fdt;
	int dtb_len;

	sboot_file = fopen(sboot_name, "rb+");
	if (sboot_file == NULL) {
		printf("update:unable to open sboot file\n");
		goto _err_use_fdt;
	}
	fseek(sboot_file, 0, SEEK_END);
	length = ftell(sboot_file);
	fseek(sboot_file, 0, SEEK_SET);
	if (!length) {
		goto _err_use_fdt;
	}
	sboot_buf = (char *)malloc(length);
	if (!sboot_buf) {
		goto _err_use_fdt;
	}
	fread(sboot_buf, length, 1, sboot_file);
	rewind(sboot_file);

	sboot_head = (sboot_file_head_t *)sboot_buf;
	ret = check_file((unsigned int *)sboot_buf,
			 sboot_head->boot_head.length, TOC0_MAGIC);
	if (ret != CHECK_IS_CORRECT) {
		goto _err_use_fdt;
	}

	/*fdt init*/
	working_fdt = probe_file_data(dtb_name, &dtb_len);
	if (working_fdt == NULL) {
		goto _err_use_fdt;
	}
	if (update_flashmap_for_boot0(sboot_head, working_fdt, storage_type) < 0) {
		printf("update from fdt failed!!!\n");
		goto _err_use_fdt;
	}
	gen_check_sum((void *)sboot_buf);
	ret = check_file((unsigned int *)sboot_buf,
			 sboot_head->boot_head.length, TOC0_MAGIC);
	if (ret != CHECK_IS_CORRECT) {
		goto _err_use_fdt;
	}
	fwrite(sboot_buf, length, 1, sboot_file);

_err_use_fdt:
	if (sboot_buf) {
		free(sboot_buf);
	}
	if (sboot_file) {
		fclose(sboot_file);
	}

	return ret;
}


int main(int argc, char *argv[])
{
	char source_sboot_name[MAX_PATH];
	char script_file_name[MAX_PATH];
	FILE *src_file   = NULL;
	int storage_type = 0;
	char *script_buf = NULL;
	char   fdt_file_name[MAX_PATH];
	int    use_fdt = 0;

	print_commit_log();
	if (argc == 3 || argc == 5) {
		if ((argv[1] == NULL) || (argv[2] == NULL)) {
			printf("update error: one of the input file names is empty\n");

			return __LINE__;
		}
		if (argc == 5) {
			use_fdt = 1;

			if ((!strcmp(argv[3], "sdmmc_card")) || (!strcmp(argv[3], "SDMMC_CARD"))) {
				storage_type = 1;
			} else if ((!strcmp(argv[3], "spinor_flash")) || (!strcmp(argv[3], "SPINOR_FLASH"))) {
				storage_type = 2;
			}
			GetFullPath(fdt_file_name, argv[4]);
			printf("fdt file =%s\n", fdt_file_name);
		}
	} else {
		Usage();

		return __LINE__;
	}
	GetFullPath(source_sboot_name, argv[1]);
	GetFullPath(script_file_name, argv[2]);

	printf("\n");
	printf("sboot file Path=%s\n", source_sboot_name);
	printf("script file Path=%s\n", script_file_name);
	printf("\n");
	if (use_fdt) {
		update_from_fdt(source_sboot_name, fdt_file_name, storage_type);
	}
	/*初始化配置脚本*/
	script_buf = (char *)script_file_decode(script_file_name);
	if (!script_buf) {
		printf("update sboot error: unable to get script data\n");

		goto _err_out;
	}
	script_parser_init(script_buf);
	/*读取原始sboot*/
	update_for_sboot(source_sboot_name, storage_type);
	/*获取原始脚本长度*/
	printf("script update sboot ok\n");
_err_out:
	if (script_buf) {
		free(script_buf);
	}

	return 0;
}

int update_for_sboot(char *sboot_name, int storage_type)
{
	FILE *sboot_file = NULL;
	sboot_file_head_t *sboot_head;
	char *sboot_buf = NULL;
	int length      = 0;
	int i;
	int ret = -1;
	int value[8];
	script_gpio_set_t gpio_set[32];
	int max_groupdram = 0;

	sboot_file = fopen(sboot_name, "rb+");
	if (sboot_file == NULL) {
		printf("update:unable to open sboot file\n");
		goto _err_sboot_out;
	}
	fseek(sboot_file, 0, SEEK_END);
	length = ftell(sboot_file);
	fseek(sboot_file, 0, SEEK_SET);
	if (!length) {
		goto _err_sboot_out;
	}
	sboot_buf = (char *)malloc(length);
	if (!sboot_buf) {
		goto _err_sboot_out;
	}
	fread(sboot_buf, length, 1, sboot_file);
	rewind(sboot_file);

	sboot_head = (sboot_file_head_t *)sboot_buf;
	/*检查sboot的数据结构是否完整*/
	ret = check_file((unsigned int *)sboot_buf,
			 sboot_head->boot_head.length, TOC0_MAGIC);
	if (ret != CHECK_IS_CORRECT) {
		goto _err_sboot_out;
	}

	if (script_parser_fetch("twi_para", "twi_used", value)) {
		value[0] = 0;
	}
	if (value[0]) {
		if (script_parser_fetch("twi_para", "twi_port", &value[1])) {
			value[1] = SUNXI_PHY_R_I2C0;
		}
		if (!script_parser_mainkey_get_gpio_cfg("twi_para", gpio_set, 32)) {
			for (i = 0; i < 32; i++) {
				if (!gpio_set[i].port) {
					break;
				}
				sboot_head->i2c_gpio[i].port		= gpio_set[i].port;
				sboot_head->i2c_gpio[i].port_num	= gpio_set[i].port_num;
				sboot_head->i2c_gpio[i].mul_sel		= gpio_set[i].mul_sel;
				sboot_head->i2c_gpio[i].pull		= gpio_set[i].pull;
				sboot_head->i2c_gpio[i].drv_level	= gpio_set[i].drv_level;
				sboot_head->i2c_gpio[i].data		= gpio_set[i].data;
				sboot_head->i2c_gpio[i].reserved[0]	= value[1];
			}
		}


	}
	if (!strncmp((char *)sboot_head->extd_head.magic, "DRAM.ext",
		     strlen("DRAM.ext"))) {
		if (!strncmp((char *)sboot_head->extd_head.version, "dram2.0",
			     strlen("dram2.0"))) {
			max_groupdram = 32;
		} else {
			max_groupdram = 15;
		}

		if (!script_parser_fetch("dram_select_para", "select_mode", value)) {
			sboot_head->extd_head.select_mode = value[0];
		}
		printf("extd_head.select_mode:%d\n", sboot_head->extd_head.select_mode);
		printf("magic:%s\n", sboot_head->extd_head.magic);
		printf("version:%s\n", sboot_head->extd_head.version);
		printf("max_groupdram:%d\n", max_groupdram);
		if (sboot_head->extd_head.select_mode != 0) {
			char dram_para_name[16];
			for (i = 1; i <= max_groupdram; i++) {
				sprintf(dram_para_name, "dram_para%d", i);
				if (script_parser_sunkey_all(
					    dram_para_name,
					    (void *)sboot_head->extd_head
						    .dram_para[i - 1])) {
					printf("script fetch dram para%d failed\n",
					       i);
				}
			}
			if (!script_parser_fetch("dram_select_para",
						 "gpadc_channel", value)) {
				sboot_head->extd_head.gpadc_channel = value[0];
			}
			for (i = 0; i < 4; i++) {
				if (!script_parser_mainkey_get_gpio_cfg(
					    "dram_select_para", gpio_set, 32)) {
					sboot_head->extd_head
						.dram_select_gpio[i]
						.port = gpio_set[i].port;
					sboot_head->extd_head
						.dram_select_gpio[i]
						.port_num =
						gpio_set[i].port_num;
					sboot_head->extd_head
						.dram_select_gpio[i]
						.mul_sel = gpio_set[i].mul_sel;
					sboot_head->extd_head
						.dram_select_gpio[i]
						.pull = gpio_set[i].pull;
					sboot_head->extd_head
						.dram_select_gpio[i]
						.drv_level =
						gpio_set[i].drv_level;
					sboot_head->extd_head
						.dram_select_gpio[i]
						.data = gpio_set[i].data;
				}
			}
		}
	}
	/*数据修正完毕*/
	/*重新计算校验和*/
	gen_check_sum((void *)sboot_buf);
	/*再检查一次*/
	ret = check_file((unsigned int *)sboot_buf,
			 sboot_head->boot_head.length, TOC0_MAGIC);
	if (ret != CHECK_IS_CORRECT) {
		goto _err_sboot_out;
	}
	fwrite(sboot_buf, length, 1, sboot_file);

_err_sboot_out:
	if (sboot_buf) {
		free(sboot_buf);
	}
	if (sboot_file) {
		fclose(sboot_file);
	}

	return ret;
}

void *script_file_decode(char *script_file_name)
{
	FILE *script_file;
	void *script_buf = NULL;
	int script_length;
	/*读取原始脚本*/
	script_file = fopen(script_file_name, "rb");
	if (!script_file) {
		printf("update error:unable to open script file\n");
		return NULL;
	}
	/*获取原始脚本长度*/
	fseek(script_file, 0, SEEK_END);
	script_length = ftell(script_file);
	if (!script_length) {
		fclose(script_file);
		printf("the length of script is zero\n");

		return NULL;
	}
	/*读取原始脚本*/
	script_buf = (char *)malloc(script_length);
	if (!script_buf) {
		fclose(script_file);
		printf("unable malloc memory for script\n");

		return NULL;
		;
	}
	fseek(script_file, 0, SEEK_SET);
	fread(script_buf, script_length, 1, script_file);
	fclose(script_file);

	return script_buf;
}
