/*
 * (C) Copyright 2022 allwinnertech  <huangrongcun@allwinnertech.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include "commit.h"
#include "./script.h"

#include "../update_uboot/script.h"
#include "../update_uboot/types.h"
#include "../update_uboot/check.h"
#include "../update_uboot/spare_head.h"
#include "../update_scp/libfdt/libfdt_env.h"
#include "../update_scp/libfdt/fdt.h"
#include "../update_scp/libfdt/libfdt.h"
#include "../update_scp/libfdt/libfdt_internal.h"
#include "../include/type_def.h"

#define MAX_PATH (260)

#define DTBIN "temp_fdt.dtb"
#define UBOOTNODTB "temp_ubootnodtb.bin"

#define SPLIT_SUCCESS     0
#define SPLIT_FAIL       -1

#define MERGE_SUCCESS     0
#define MERGE_FAIL       -1

#define DEBUG
#ifdef DEBUG
#define debugf(fmt, args...)                                                   \
	do {                                                                   \
		printf("%s(): LINE:%d ", __func__, __LINE__);                                    \
		printf(fmt, ##args);                                           \
	} while (0)
#else
#define debugf(fmt, args...)
#endif

__s32 check_magic(void *mem_base, const char *magic);

int align_size;

static void usage(void)
{
	printf(" sunxi_ubootools length uboot_nodtb.bin\n");
	printf(" sunxi_ubootools split uboot.bin\n");
	printf(" sunxi_ubootools merge uboot_nodtb dtb\n");
	printf(" sunxi_ubootools sys_config.bin mainkey subkey\n");
	return;
}

int IsFullName(const char *FilePath)
{
	if (isalpha(FilePath[0]) && ':' == FilePath[1])
		return 1;
	else
		return 0;
}

void GetFullPath(char *dName, const char *sName)
{
	char Buffer[256];

	if (IsFullName(sName)) {
		strcpy(dName, sName);
		return;
	}

	/* Get the current working directory: */
	if (getcwd(Buffer, 256) == NULL) {
		perror("getcwd error");
		return;
	}
	sprintf(dName, "%s/%s", Buffer, sName);
}

int update_for_uboot(char *uboot_name)
{
	FILE *uboot_file = NULL;
	struct spare_boot_head_t *uboot_head;
	struct extend_boot_head_400_t *uboot_400_head;
	char *uboot_buf = NULL;
	int length = 0;
	int i;
	int ret = -1;
	uboot_file = fopen(uboot_name, "rb+");

	if (uboot_file == NULL) {
		printf("update uboot error : unable to open uboot file\n");
		goto _err_uboot_out;
	}

	fseek(uboot_file, 0, SEEK_END);
	length = ftell(uboot_file);
	fseek(uboot_file, 0, SEEK_SET);

	if (!length) {
		printf("update uboot error : uboot length is zero\n");
		goto _err_uboot_out;
	}
	length = (length + 3) / 4 * 4;

	uboot_buf = (char *)malloc(length);

	if (!uboot_buf) {
		printf("update uboot error : fail to malloc memory for uboot\n");
		goto _err_uboot_out;
	}

	fread(uboot_buf, length, 1, uboot_file);
	rewind(uboot_file);
	ret = check_magic((unsigned int *)uboot_buf, UBOOT_MAGIC);
	if (ret) {
		printf("write uboot length: check magic error\n");
		goto _err_uboot_out;
	}
	uboot_head = (struct spare_boot_head_t *)uboot_buf;
	uboot_head->boot_data.dtb_offset = length;
	printf("uboot_head->boot_head.dtb_offset=%d\n",
	       uboot_head->boot_data.dtb_offset);
	fwrite(uboot_buf, length, 1, uboot_file);

_err_uboot_out:
	if (uboot_buf) {
		free(uboot_buf);
	}

	if (uboot_file) {
		fclose(uboot_file);
	}
	return 0;
}

int split_ubootfdt(char *uboot_name)
{
	FILE *uboot_file = NULL;
	FILE *temp_uboot_file = NULL, *temp_fdt_file = NULL;
	struct spare_boot_head_t *uboot_head;
	struct fdt_header *working_fdt;
	char *uboot_buf = NULL;
	u32 length = 0, dtb_offset = 0;
	int i;
	int ret = -1;
	int flag = SPLIT_FAIL;

	uboot_file = fopen(uboot_name, "rb+");

	if (uboot_file == NULL) {
		printf("split uboot error : unable to open uboot file\n");
		goto _err_uboot_out;
	}

	fseek(uboot_file, 0, SEEK_END);
	length = ftell(uboot_file);
	fseek(uboot_file, 0, SEEK_SET);

	if (!length) {
		printf("update uboot error : uboot length is zero\n");
		goto _err_uboot_out;
	}
	length = (length + 3) / 4 * 4;
	uboot_buf = (char *)malloc(length);

	if (!uboot_buf) {
		printf("update uboot error : fail to malloc memory for uboot\n");
		goto _err_uboot_out;
	}

	fread(uboot_buf, length, 1, uboot_file);
	rewind(uboot_file);
	ret = check_magic((unsigned int *)uboot_buf, UBOOT_MAGIC);
	if (ret) {
		printf("split error : check magic error\n");
		goto _err_uboot_out;
	}

	uboot_head = (struct spare_boot_head_t *)uboot_buf;
	dtb_offset = uboot_head->boot_data.dtb_offset;
	if (!dtb_offset) {
		printf("error:uboot length = %d please check uboot head\n", dtb_offset);
		goto _err_uboot_out;
	}
	debugf("uboot_head->boot_head.dtb_offset=%d\n",
	       uboot_head->boot_data.dtb_offset);

	if (access(UBOOTNODTB, F_OK) == 0) {
		remove(UBOOTNODTB);
	}
	temp_uboot_file = fopen(UBOOTNODTB, "wb");
	if (temp_uboot_file == NULL) {
		printf("create  temp_uboot_file failed!!!\n");
		goto _err_uboot_out;
	}
	printf("write UBOOTNODTB file OK \n");
	fwrite(uboot_buf, dtb_offset, 1, temp_uboot_file);

	working_fdt = (struct fdt_header *)(uboot_buf + dtb_offset);
	debugf("uboot uboot_buf addr = %p\n", uboot_buf);
	debugf("working_fdt addr =%p\n", working_fdt);

	if ((fdt_check_header(working_fdt)) != 0) {
		printf("check fdt error\nplease check the bin file!!!\n");
		goto _err_uboot_out;
	}
	debugf("check fdt sucess\n");
	if (access(DTBIN, F_OK) == 0) {
		remove(DTBIN);
	}
	temp_fdt_file = fopen(DTBIN, "wb");
	if (temp_fdt_file == NULL) {
		printf("create  temp_fdt_file failed!!!\n");
		goto _err_uboot_out;
	}
	fwrite(working_fdt, length - dtb_offset, 1, temp_fdt_file);
	printf("write DTBIN file OK \n");
	flag = SPLIT_SUCCESS;
_err_uboot_out:

	if (uboot_buf) {
		free(uboot_buf);
	}
	if (uboot_file) {
		fclose(uboot_file);
	}
	if (temp_uboot_file) {
		fclose(temp_uboot_file);
	}
	if (temp_fdt_file) {
		fclose(temp_fdt_file);
	}
	return flag;
}

int merge_ubootfdt(char *uboot_nodtb_name, char *dtb_name)
{
	FILE *uboot_file = NULL, *dtb_file = NULL;
	FILE *temp_uboot_file = NULL, *temp_fdt_file = NULL;
	struct spare_boot_head_t *uboot_head;
	struct fdt_header *working_fdt;
	char *uboot_buf = NULL, *uboot_nodtb_buf = NULL;
	u32 length = 0, dtb_offset = 0, dtb_length = 0;
	int i;
	int ret = -1;
	int flag = MERGE_FAIL;

	uboot_file = fopen(uboot_nodtb_name, "rb+");
	if (uboot_file == NULL) {
		printf("merge uboot error : unable to open uboot file\n");
		goto _err_uboot_out;
	}

	dtb_file = fopen(dtb_name, "rb+");

	if (dtb_file == NULL) {
		printf("merge uboot error : unable to open dtb file\n");
		goto _err_uboot_out;
	}
	rewind(uboot_file);
	fseek(uboot_file, 0, SEEK_END);
	length = ftell(uboot_file);
	fseek(uboot_file, 0, SEEK_SET);

	if (!length) {
		printf("update uboot error : uboot length is zero\n");
		goto _err_uboot_out;
	}

	length = (length + 3) / 4 * 4;
	debugf("length------>%d\n", length);
	uboot_nodtb_buf = (char *)malloc(length);

	if (!uboot_nodtb_buf) {
		printf("update uboot error : fail to malloc memory for uboot\n");
		goto _err_uboot_out;
	}

	fread(uboot_nodtb_buf, length, 1, uboot_file);
	ret = check_magic((unsigned int *)uboot_nodtb_buf, UBOOT_MAGIC);
	if (ret) {
		printf("merge uboot : check magic error\n");
		goto _err_uboot_out;
	}
	uboot_head = (struct spare_boot_head_t *)uboot_nodtb_buf;
	align_size = uboot_head->boot_head.align_size;

	/* get uboot nodtb length*/
	dtb_offset = uboot_head->boot_data.dtb_offset;
	if (!dtb_offset) {
		printf("uboot length error,please check uboot head\n");
		goto _err_uboot_out;
	} else if (dtb_offset != length) {
		printf("warning:uboot length and header record length are inconsistent\n");
		printf("use length of no dtb bin\n");
		uboot_head->boot_data.dtb_offset = length;
	}


	/* get dtb length*/
	fseek(dtb_file, 0, SEEK_END);
	dtb_length = ftell(dtb_file);
	fseek(dtb_file, 0, SEEK_SET);

	if (!dtb_length) {
		printf("merge error : dtb length is zero\n");
		goto _err_uboot_out;
	}
	dtb_length = (dtb_length + 3) / 4 * 4;
	debugf("uboot_length=0x%x,dtb_length = 0x%x\n",
	       length, dtb_length);
	/* all length = uboot length + dtb length */
	uboot_buf = (char *)malloc(length + dtb_length);
	if (!uboot_buf) {
		printf("merge uboot error : fail to malloc memory for uboot\n");
		goto _err_uboot_out;
	}

	rewind(uboot_file);
	rewind(dtb_file);

	fread(uboot_buf, length, 1, uboot_file);
	fread(uboot_buf+length, dtb_length, 1, dtb_file);
	rewind(uboot_file);
	fwrite(uboot_buf, length + dtb_length, 1, uboot_file);
	printf("write DTBIN to uboot sucess!!! \n");
	flag = MERGE_SUCCESS;

_err_uboot_out:

	if (uboot_buf) {
		free(uboot_buf);
	}
	if (uboot_nodtb_buf) {
		free(uboot_nodtb_buf);
	}
	if (uboot_file) {
		fclose(uboot_file);
	}
	if (dtb_file) {
		fclose(dtb_file);
	}
	return flag;
}

int get_subkey_value_u32(char *file_sysconfig, char *mainkey, char *subkey)
{
	char script_file_name[MAX_PATH];
	char *script_buf = NULL;
	int value;
	GetFullPath(script_file_name, file_sysconfig);
	script_buf = (char *)script_file_decode(script_file_name);
	if (!script_buf) {
		printf("decode err: unable to get script data\n");
		free(script_buf);
		goto _err_get_subkey;
	}
	printf("mainkey:%s subkey:%s\n", mainkey, subkey);
	script_parser_init(script_buf);
	if (!script_parser_fetch(mainkey, subkey, &value)) {
		printf("value = %d\n", value);
		return value;
	}
	printf("error: read failed\n");
_err_get_subkey:
	if (script_buf)
		free(script_buf);
	return -1;
}

int main(int argc, char *argv[])
{
	char ubootpath[MAX_PATH] = "";
	char cmd[MAX_PATH] = "";
	char *working_uboot;
	char *working_fdt;

	print_commit_log();
	if (argc < 3 || argc > 5) {
		printf("parameters invalid\n");
		usage();
		return -1;
	}
	if (!strcmp(argv[1], "ubootlength")) {
		update_for_uboot(argv[2]);
	} else if (!strcmp(argv[1], "split")) {
		if (split_ubootfdt(argv[2]) < 0) {
			printf("split error: file:%s,line:%d\n", __FILE__, __LINE__);
			return -1;
		}
	} else if (!strcmp(argv[1], "merge")) {
		if (merge_ubootfdt(argv[2], argv[3]) < 0) {
			printf("merge error: file:%s,line:%d\n", __FILE__, __LINE__);
			return -1;
		}
	} else if (!strcmp(argv[1], "subkey_value")) {
		if (argc == 5) {
			return get_subkey_value_u32(argv[2], argv[3], argv[4]);
		} else {
			printf("Parameter error\n");
			return 0;
		}

	} else {
		printf("cant find cmd: %s \n", argv[2]);
	}
	return 0;
}
