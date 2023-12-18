/*
**********************************************************************************************************************
*											        eGon
*						                     the Embedded System
*									       boot pack sub-system
*
*						  Copyright(C), 2006-2017, SoftWinners Microelectronic Co., Ltd.
*                                           All Rights Reserved
*
* File    : script.c
*
* Version : V2.00
*
* Date	  :
*
* Descript:
**********************************************************************************************************************
*/
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>

#include "sunxi_mbr.h"
#include "script.h"
#include "types.h"
#include "file.h"
#include "part.h"
#include "commit.h"

int main(int argc, char *argv[])
{
	char part_name[MAX_PATH];
	char mbr_name[MAX_PATH];
	char img_name[MAX_PATH];
	char pro_img_name[MAX_PATH];
	char gpt_name[MAX_PATH];

	char *pbuf = NULL;
	char *mbr_buf = NULL;

	sunxi_mbr_t *mbr_info = NULL;

	FILE *img_file = NULL;
	FILE *part_file = NULL;
	FILE *mbr_file = NULL;

	int ret = 0;
	int script_len = 0;
	unsigned int mbr_size = 0;
	int use_gpt = 0;


	print_commit_log();

	memset(part_name, 0, sizeof(part_name));
	memset(mbr_name, 0, sizeof(mbr_name));
	memset(pro_img_name, 0, sizeof(pro_img_name));
	memset(img_name, 0, sizeof(img_name));
	memset(gpt_name, 0, sizeof(gpt_name));

	if (argc == 4) {
		GetFullPath(part_name, argv[1]); /* boot0, toc0 */
		GetFullPath(mbr_name, argv[2]);  /* uboot,toc1 */
		GetFullPath(pro_img_name,
			    argv[3]); /* img_to_programmer,ouput file */
	} else if (argc == 5) {
		GetFullPath(part_name, argv[1]);    /* sys_partition.bin */
		GetFullPath(mbr_name, argv[2]);     /* sunxi_mbr.fex */
		GetFullPath(pro_img_name, argv[3]); /* img_to_programmer */
		memcpy(img_name, argv[4],
		       MAX_PATH); /* img name to sysrecovery */

		set_img_name(img_name);
	} else if (argc == 6) {
		GetFullPath(part_name, argv[1]);    /* sys_partition.bin */
		GetFullPath(mbr_name, argv[2]);     /* sunxi_mbr.fex */
		GetFullPath(pro_img_name, argv[3]); /* img_to_programmer */
		memcpy(img_name, argv[4],
		       MAX_PATH); /* img name to sysrecovery */
		GetFullPath(gpt_name, argv[5]); /* sunxi_gpt.fex*/
		set_img_name(img_name);
		use_gpt = 1;
		printf("gpt path=%s\n", gpt_name);
	} else {
		printf("invalid input\n");
		return 0;
	}

	printf("part_name=%s\n", part_name);
	printf("mbr_name=%s\n", mbr_name);
	printf("pro_img_name path=%s\n", pro_img_name);
	printf("img_name=%s\n", img_name);

	if (argc == 4) {
		/* clear file and create 20M zero file */
		img_file = fopen(pro_img_name, "wb");
		if (!img_file) {
			printf("open file %s failed\n", pro_img_name);
			goto _err_out;
		}

		ret = fill_file_data(img_file, (20 * 1024 * 1024));
		if (ret) {
			printf("fill file %s failed\n", pro_img_name);
			goto _err_out;
		}
		fclose(img_file);
		img_file = NULL;

		img_file = fopen(pro_img_name, "rb+");
		if (!img_file) {
			printf("open file %s failed\n", pro_img_name);
			goto _err_out;
		}

		if (write_boot_part(part_name, mbr_name, img_file)) {
			printf("write boot file failed\n");
			goto _err_out;
		}
		printf("+++write boot file sucess+++\n");
	} else {
		part_file = fopen(part_name, "rb");
		if (!part_file) {
			printf("unable to open script file\n");

			goto _err_out;
		}
		/* 读出脚本的数据 */
		/* 首先获取脚本的长度 */
		fseek(part_file, 0, SEEK_END);
		script_len = ftell(part_file);
		fseek(part_file, 0, SEEK_SET);
		/* 读出脚本所有的内? */
		pbuf = (char *)malloc(script_len);
		if (!pbuf) {
			printf("unable to malloc memory for script\n");

			goto _err_out;
		}
		memset(pbuf, 0, script_len);
		fread(pbuf, 1, script_len, part_file);
		fclose(part_file);
		part_file = NULL;
		/* script使用初始化 */
		if (SCRIPT_PARSER_OK != script_parser_init(pbuf)) {
			goto _err_out;
		}

		img_file = fopen(pro_img_name, "rb+");
		if (!img_file) {
			printf("open file %s failed\n", pro_img_name);
			goto _err_out;
		}


		if (script_parser_fetch("mbr", "size", &mbr_size) ||
		    (!mbr_size)) {
			mbr_size = 16384;
		}
		printf("mbr size = %d kbytes\n", mbr_size);
		mbr_size = mbr_size * 1024 / 512;//unit:sector

		ret = write_mbr_file(mbr_name, img_file, mbr_size);
		if (ret) {
			printf("write file %s failed\n", mbr_name);
			goto _err_out;
		}

		mbr_file = fopen(mbr_name, "rb");
		if (!mbr_file) {
			printf("can't open %s file\n", mbr_name);
			goto _err_out;
		}

		mbr_buf = malloc(sizeof(sunxi_mbr_t));
		if (!mbr_buf) {
			printf("can't malloc memory for sunxi_mbr\n");
			goto _err_out;
		}

		memset(mbr_buf, 0, sizeof(sunxi_mbr_t));

		fseek(mbr_file, 0, SEEK_SET);
		fread(mbr_buf, 1, sizeof(sunxi_mbr_t), mbr_file);
		mbr_info = (sunxi_mbr_t *)mbr_buf;
		fclose(mbr_file);
		mbr_file = NULL;

		ret = write_normal_part2img(img_file, mbr_info, mbr_size);
		if (ret) {
			printf("write normal part err\n");
		}

		printf("use_gpt = %d\n", use_gpt);
		if (use_gpt) {
			ret = write_gpt_file(gpt_name, img_file);
			if (ret) {
				printf("write gpt err\n");
			}
		} else {
			ret = write_standar_mbr_file(mbr_info, img_file);
			if (ret) {
				printf("write standard mbr err\n");
			}
		}

		script_parser_exit();
	}

_err_out:
	if (pbuf)
		free(pbuf);
	if (mbr_buf)
		free(mbr_buf);
	if (img_file)
		fclose(img_file);
	if (mbr_file)
		fclose(mbr_file);

	return 0;
}


